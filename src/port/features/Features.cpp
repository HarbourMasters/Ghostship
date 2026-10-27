#include "Features.h"

#include <algorithm>
#include <cmath>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>
#include <libultraship/bridge/consolevariablebridge.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>

#include "port/ui/cvar_prefixes.h"

namespace Features {

enum class Kind {
    Toggle,
    Int,
    Float,
    // A button or window; it can be disabled but has no value to force.
    Action,
};

struct Def {
    const char* id;
    const char* cvar;
    Kind kind;
    // What reads return while the feature is disabled: the vanilla game's behaviour.
    float value;
};

// settings.* (audio, controls, menu, renderer, the romhack and mod selection) is never lockable.
static const Def sDefs[] = {
    { "cheats.infinite_health", CVAR_CHEAT("InfiniteHealth"), Kind::Toggle, 0 },
    { "cheats.infinite_lives", CVAR_CHEAT("InfiniteLives"), Kind::Toggle, 0 },
    { "cheats.always_fly_triple_jump", CVAR_CHEAT("AlwaysFlyTripleJump"), Kind::Toggle, 0 },
    { "cheats.flying_triple_jump_high_launch", CVAR_CHEAT("FlyingTripleJumpHighLaunch"), Kind::Toggle, 0 },
    { "cheats.pause_exit_whenever", CVAR_CHEAT("PauseExitWhenever"), Kind::Toggle, 0 },
    { "cheats.play_in_demo", CVAR_CHEAT("PlayInDemo"), Kind::Toggle, 0 },

    { "enhancements.disable_draw_distance", CVAR_ENHANCEMENT("DisableDrawDistance"), Kind::Toggle, 0 },
    { "enhancements.disable_lod", CVAR_ENHANCEMENT("DisableLOD"), Kind::Toggle, 0 },
    { "enhancements.disable_lakitu_cutscene", CVAR_ENHANCEMENT("DisableLakituCutscene"), Kind::Toggle, 0 },
    { "enhancements.disable_peach_cutscene", CVAR_ENHANCEMENT("DisablePeachCutscene"), Kind::Toggle, 0 },
    { "enhancements.select_all_stars", CVAR_ENHANCEMENT("SelectAllStars"), Kind::Toggle, 0 },
    { "enhancements.star_no_exit", CVAR_ENHANCEMENT("StarNoExit"), Kind::Toggle, 0 },
    { "enhancements.always_show_power_meter", CVAR_ENHANCEMENT("AlwaysShowPowerMeter"), Kind::Toggle, 0 },
    { "enhancements.mirrored_world", CVAR_ENHANCEMENT("Modes.MirroredWorld.Mode"), Kind::Int, 0 },
    { "enhancements.fix_koopa_race_music", CVAR_ENHANCEMENT("FixKoopaRaceMusic"), Kind::Toggle, 0 },
    { "enhancements.fix_heave_ho_sinking", CVAR_ENHANCEMENT("FixHeaveHoSinkingThroughFloor"), Kind::Toggle, 0 },
    { "enhancements.fix_yoshi_dialogue_typo", CVAR_ENHANCEMENT("FixYoshiDialogueTypo"), Kind::Toggle, 0 },
    { "enhancements.fix_file_select_button_tilt", CVAR_ENHANCEMENT("FixFileSelectButtonTilt"), Kind::Toggle, 0 },

    { "camera.default_mode", CVAR_ENHANCEMENT("Camera.DefaultMode"), Kind::Int, 0 },
    { "camera.alternate_mode", CVAR_ENHANCEMENT("Camera.AlternateMode"), Kind::Int, 0 },
    { "camera.improved_c_buttons", CVAR_ENHANCEMENT("Camera.ImprovedCButtonCamera"), Kind::Toggle, 0 },
    { "camera.horizontal_analog", CVAR_ENHANCEMENT("Camera.HorizontalAnalog"), Kind::Toggle, 0 },
    { "camera.vertical_analog", CVAR_ENHANCEMENT("Camera.VerticalAnalog"), Kind::Toggle, 0 },
    { "camera.center_button", CVAR_ENHANCEMENT("Camera.CenterCameraButton"), Kind::Toggle, 0 },

    { "graphics.alt_assets", CVAR_ENHANCEMENT("Mods.AlternateAssets"), Kind::Toggle, 0 },
    { "graphics.interpolation_fps", CVAR_SETTING("InterpolationFPS"), Kind::Int, 30 },
    { "graphics.match_refresh_rate", CVAR_SETTING("MatchRefreshRate"), Kind::Toggle, 0 },

    { "rando.enabled", CVAR_RANDOMIZER_SETTING("Enabled"), Kind::Toggle, 0 },

    { "devtools.debug_level_select", CVAR_DEVELOPER_TOOLS("DebugMode"), Kind::Toggle, 0 },
    { "devtools.better_level_select", CVAR_DEVELOPER_TOOLS("BetterLevelSelect"), Kind::Toggle, 0 },
    { "devtools.save_editor", CVAR_WINDOW("SaveEditor"), Kind::Action, 0 },
    { "devtools.object_viewer", CVAR_WINDOW("ObjectViewer"), Kind::Action, 0 },
};

struct Value {
    int32_t i = 0;
    float f = 0.0f;

    bool operator==(const Value& o) const {
        return i == o.i && f == o.f;
    }
};

struct State {
    const Def* def = nullptr;
    std::vector<std::string> disabledBy;
    std::optional<Value> forced;
    std::string forcedBy;
};

using StateMap = std::unordered_map<std::string_view, State>;

static StateMap& States() {
    static StateMap states = []() {
        StateMap map;
        for (const Def& def : sDefs) {
            State state;
            state.def = &def;
            map.emplace(def.id, std::move(state));
        }
        return map;
    }();
    return states;
}

static const std::unordered_map<std::string, const char*>& CVarIndex() {
    static const std::unordered_map<std::string, const char*> index = []() {
        std::unordered_map<std::string, const char*> map;
        for (const Def& def : sDefs) {
            if (def.cvar != nullptr) {
                map.emplace(def.cvar, def.id);
            }
        }
        return map;
    }();
    return index;
}

static std::vector<Conflict> sConflicts;

static const State* Find(const char* id) {
    if (id == nullptr) {
        return nullptr;
    }
    StateMap& states = States();
    const auto it = states.find(std::string_view(id));
    if (it != states.end()) {
        return &it->second;
    }
    static std::unordered_set<std::string> sWarned;
    if (sWarned.insert(id).second) {
        SPDLOG_WARN("Features: unknown feature id {}", id);
    }
    return nullptr;
}

static std::vector<State*> Match(const std::string& pattern) {
    std::vector<State*> out;
    if (pattern.rfind("settings.", 0) == 0) {
        return out;
    }
    const bool wildcard = pattern.size() >= 2 && pattern.compare(pattern.size() - 2, 2, ".*") == 0;
    const std::string prefix = wildcard ? pattern.substr(0, pattern.size() - 1) : pattern;
    for (auto& [id, state] : States()) {
        if (wildcard ? id.rfind(prefix, 0) == 0 : id == prefix) {
            out.push_back(&state);
        }
    }
    return out;
}

static std::optional<Value> Convert(const Def& def, const nlohmann::json& json) {
    Value v;
    switch (def.kind) {
        case Kind::Toggle:
        case Kind::Int:
            if (json.is_boolean()) {
                v.i = json.get<bool>() ? 1 : 0;
            } else if (json.is_number()) {
                v.i = (int32_t)std::lround(json.get<double>());
            } else {
                return std::nullopt;
            }
            v.f = (float)v.i;
            return v;
        case Kind::Float:
            if (!json.is_number()) {
                return std::nullopt;
            }
            v.f = json.get<float>();
            v.i = (int32_t)v.f;
            return v;
        case Kind::Action:
            return std::nullopt;
    }
    return std::nullopt;
}

static std::string FormatValue(const Def& def, const Value& v) {
    switch (def.kind) {
        case Kind::Toggle:
            return v.i != 0 ? "on" : "off";
        case Kind::Int:
            return std::to_string(v.i);
        case Kind::Float:
            return fmt::format("{:g}", v.f);
        case Kind::Action:
            break;
    }
    return "";
}

void Resolve(const std::vector<RuleSource>& sources) {
    for (auto& [id, state] : States()) {
        state.disabledBy.clear();
        state.forced.reset();
        state.forcedBy.clear();
    }
    sConflicts.clear();

    for (const RuleSource& source : sources) {
        for (const std::string& pattern : source.disable) {
            const auto matches = Match(pattern);
            if (matches.empty()) {
                SPDLOG_WARN("Features: {} disables unknown feature {}", source.name, pattern);
            }
            for (State* state : matches) {
                if (std::find(state->disabledBy.begin(), state->disabledBy.end(), source.name) ==
                    state->disabledBy.end()) {
                    state->disabledBy.push_back(source.name);
                }
            }
        }
        for (const auto& [pattern, json] : source.force) {
            const auto matches = Match(pattern);
            if (matches.empty()) {
                SPDLOG_WARN("Features: {} forces unknown feature {}", source.name, pattern);
            }
            for (State* state : matches) {
                const auto value = Convert(*state->def, json);
                if (!value.has_value()) {
                    SPDLOG_WARN("Features: {} forces {} to {}, which it cannot take", source.name, state->def->id,
                                json.dump());
                    continue;
                }
                if (state->forced.has_value() && state->forcedBy != source.name && !(*state->forced == *value)) {
                    SPDLOG_WARN("Features: {} forces {} to {}, overriding {} ({})", source.name, state->def->id,
                                FormatValue(*state->def, *value), state->forcedBy,
                                FormatValue(*state->def, *state->forced));
                    sConflicts.push_back({ state->def->id, source.name, state->forcedBy });
                }
                state->forced = value;
                state->forcedBy = source.name;
            }
        }
    }

    for (const Def& def : sDefs) {
        if (IsLocked(def.id)) {
            SPDLOG_INFO("Features: {} locked: {}", def.id, LockReason(def.id));
        }
    }
}

bool IsLocked(const char* id) {
    const State* state = Find(id);
    return state != nullptr && (state->forced.has_value() || !state->disabledBy.empty());
}

std::string LockReason(const char* id) {
    const State* state = Find(id);
    if (state == nullptr) {
        return "";
    }
    if (state->forced.has_value()) {
        if (state->def->kind == Kind::Toggle) {
            return fmt::format("Turned {} by {}", FormatValue(*state->def, *state->forced), state->forcedBy);
        }
        return fmt::format("Set to {} by {}", FormatValue(*state->def, *state->forced), state->forcedBy);
    }
    if (state->disabledBy.empty()) {
        return "";
    }
    std::string names;
    for (const std::string& name : state->disabledBy) {
        names += (names.empty() ? "" : ", ") + name;
    }
    return "Disabled by " + names;
}

const char* ForCVar(const char* cvar) {
    if (cvar == nullptr || cvar[0] == '\0') {
        return nullptr;
    }
    const auto& index = CVarIndex();
    const auto it = index.find(cvar);
    return it != index.end() ? it->second : nullptr;
}

const std::vector<Conflict>& Conflicts() {
    return sConflicts;
}

bool DrawLockedNotice(const char* id) {
    if (Port_FeatureAllowed(id)) {
        return false;
    }
    ImGui::TextDisabled("%s.", LockReason(id).c_str());
    return true;
}

} // namespace Features

using namespace Features;

static Value Effective(const State* state, Value cvarValue) {
    if (state->forced.has_value()) {
        return *state->forced;
    }
    if (!state->disabledBy.empty()) {
        Value v;
        v.i = (int32_t)state->def->value;
        v.f = state->def->value;
        return v;
    }
    return cvarValue;
}

extern "C" bool Port_FeatureAllowed(const char* id) {
    const State* state = Find(id);
    return state == nullptr || state->disabledBy.empty();
}

extern "C" int32_t Port_FeatureInt(const char* id, int32_t fallback) {
    const State* state = Find(id);
    if (state == nullptr) {
        return fallback;
    }
    Value v;
    v.i = state->def->cvar != nullptr ? CVarGetInteger(state->def->cvar, fallback) : fallback;
    return Effective(state, v).i;
}

extern "C" float Port_FeatureFloat(const char* id, float fallback) {
    const State* state = Find(id);
    if (state == nullptr) {
        return fallback;
    }
    Value v;
    v.f = state->def->cvar != nullptr ? CVarGetFloat(state->def->cvar, fallback) : fallback;
    return Effective(state, v).f;
}

extern "C" const char* Port_FeatureDisabledBy(const char* id) {
    const State* state = Find(id);
    return state != nullptr && !state->disabledBy.empty() ? state->disabledBy.front().c_str() : nullptr;
}
