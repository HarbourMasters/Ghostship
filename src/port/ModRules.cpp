#include "port/ModRules.h"

#include <algorithm>
#include <cctype>

#include <ship/resource/File.h>
#include <ship/resource/archive/Archive.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "port/ModList.h"
#include "port/features/Features.h"
#include "port/romhack/Romhack.h"

namespace ModRules {

static std::vector<std::string> StringList(const nlohmann::json& node) {
    std::vector<std::string> out;
    if (node.is_string()) {
        out.push_back(node.get<std::string>());
    } else if (node.is_array()) {
        for (const auto& item : node) {
            if (item.is_string()) {
                out.push_back(item.get<std::string>());
            }
        }
    }
    return out;
}

static std::optional<BaseRef> ParseBaseRef(const nlohmann::json& node) {
    if (node.is_string()) {
        return BaseRef{ node.get<std::string>(), "" };
    }
    if (node.is_object() && node.contains("name") && node["name"].is_string()) {
        return BaseRef{ node["name"].get<std::string>(), node.value("version", "") };
    }
    return std::nullopt;
}

Rules Parse(const nlohmann::json& manifest) {
    Rules rules;
    if (!manifest.is_object()) {
        return rules;
    }
    if (const auto it = manifest.find("works_with"); it != manifest.end()) {
        std::vector<BaseRef> bases;
        const nlohmann::json list = it->is_array() ? *it : nlohmann::json::array({ *it });
        for (const auto& item : list) {
            if (auto ref = ParseBaseRef(item)) {
                bases.push_back(std::move(*ref));
            }
        }
        rules.worksWith = std::move(bases);
    }
    if (const auto it = manifest.find("disable"); it != manifest.end()) {
        rules.disable = StringList(*it);
    }
    if (const auto it = manifest.find("force"); it != manifest.end() && it->is_object()) {
        for (const auto& [key, value] : it->items()) {
            rules.force.emplace_back(key, value);
        }
    }
    rules.isRomhack = manifest.contains("romhack");
    return rules;
}

void Write(const Rules& rules, nlohmann::ordered_json& manifest) {
    if (rules.worksWith.has_value()) {
        nlohmann::ordered_json bases = nlohmann::ordered_json::array();
        for (const BaseRef& ref : *rules.worksWith) {
            if (ref.versions.empty()) {
                bases.push_back(ref.name);
            } else {
                bases.push_back({ { "name", ref.name }, { "version", ref.versions } });
            }
        }
        manifest["works_with"] = bases;
    }
    if (!rules.disable.empty()) {
        manifest["disable"] = rules.disable;
    }
    if (!rules.force.empty()) {
        manifest["force"] = nlohmann::ordered_json::object();
        for (const auto& [key, value] : rules.force) {
            manifest["force"][key] = value;
        }
    }
}

std::optional<nlohmann::json> ReadManifest(Ship::Archive& archive) {
    auto file = archive.LoadFile("manifest.json");
    if (file == nullptr || !file->IsLoaded || file->Buffer == nullptr || file->Buffer->size() <= file->BufferOffset) {
        return std::nullopt;
    }
    try {
        return nlohmann::json::parse(file->Buffer->begin() + file->BufferOffset, file->Buffer->end());
    } catch (const std::exception& e) {
        SPDLOG_WARN("Mod: unreadable manifest.json in {}: {}", archive.GetPath(), e.what());
        return std::nullopt;
    }
}

static std::vector<int64_t> VersionParts(const std::string& version) {
    std::vector<int64_t> parts;
    size_t pos = 0;
    while (pos <= version.size()) {
        const size_t dot = std::min(version.find('.', pos), version.size());
        int64_t value = 0;
        for (size_t i = pos; i < dot && std::isdigit(static_cast<unsigned char>(version[i])); i++) {
            value = value * 10 + (version[i] - '0');
        }
        parts.push_back(value);
        pos = dot + 1;
    }
    return parts;
}

static int CompareVersions(const std::string& a, const std::string& b) {
    const auto pa = VersionParts(a);
    const auto pb = VersionParts(b);
    for (size_t i = 0; i < std::max(pa.size(), pb.size()); i++) {
        const int64_t x = i < pa.size() ? pa[i] : 0;
        const int64_t y = i < pb.size() ? pb[i] : 0;
        if (x != y) {
            return x < y ? -1 : 1;
        }
    }
    return 0;
}

bool VersionInRange(const std::string& version, const std::string& range) {
    std::vector<std::string> tokens;
    std::string current;
    for (const char c : range) {
        if (c == ' ' || c == ',') {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }

    for (size_t i = 0; i < tokens.size(); i++) {
        const std::string& token = tokens[i];
        size_t opLength = 0;
        while (opLength < token.size() &&
               (token[opLength] == '<' || token[opLength] == '>' || token[opLength] == '=')) {
            opLength++;
        }
        std::string op = token.substr(0, opLength);
        std::string target = token.substr(opLength);
        if (target.empty() && i + 1 < tokens.size()) {
            target = tokens[++i];
        }
        if (op.empty()) {
            op = ">=";
        }
        const int cmp = CompareVersions(version, target);
        const bool ok = op == ">="   ? cmp >= 0
                        : op == ">"  ? cmp > 0
                        : op == "<=" ? cmp <= 0
                        : op == "<"  ? cmp < 0
                                     : cmp == 0;
        if (!ok) {
            return false;
        }
    }
    return true;
}

static std::string BaseLabel(const std::string& name) {
    if (name == kBaseVanilla) {
        return "Super Mario 64";
    }
    return name == kBaseAnyRomhack ? "a romhack" : name;
}

std::optional<std::string> IncompatibleReason(const Rules& rules) {
    if (rules.isRomhack) {
        return std::string("This is a romhack. Pick it as the base game in Mods instead of the mods folder.");
    }
    if (!rules.worksWith.has_value() || rules.worksWith->empty()) {
        return std::nullopt;
    }
    const Romhack::Info* romhack = Romhack::Active();
    const std::string baseName = romhack != nullptr ? romhack->name : kBaseVanilla;
    const std::string baseVersion = romhack != nullptr ? romhack->version : "";
    for (const BaseRef& ref : *rules.worksWith) {
        if (ref.name == "*" || (ref.name == kBaseAnyRomhack && romhack != nullptr)) {
            return std::nullopt;
        }
        const bool named =
            ref.name == baseName || (romhack != nullptr && !romhack->id.empty() && ref.name == romhack->id);
        if (named && (ref.versions.empty() || VersionInRange(baseVersion, ref.versions))) {
            return std::nullopt;
        }
    }
    std::string targets;
    for (const BaseRef& ref : *rules.worksWith) {
        targets +=
            (targets.empty() ? "" : ", ") + BaseLabel(ref.name) + (ref.versions.empty() ? "" : " " + ref.versions);
    }
    const std::string current = BaseLabel(baseName) + (baseVersion.empty() ? "" : " " + baseVersion);
    return fmt::format("Made for {}, not {}.", targets, current);
}

void Resolve() {
    std::vector<Features::RuleSource> sources;
    if (const Romhack::Info* romhack = Romhack::Active()) {
        // The randomizer shuffles the base game's checks, which a romhack has replaced.
        sources.push_back({ romhack->name, { "rando.*" }, {} });
        sources.push_back({ romhack->name, romhack->rules.disable, romhack->rules.force });
    }
    for (const ModList::Entry& entry : ModList::Entries()) {
        if (entry.mounted) {
            sources.push_back({ entry.archive->GetManifest().Name, entry.rules.disable, entry.rules.force });
        }
    }
    Features::Resolve(sources);
}

} // namespace ModRules
