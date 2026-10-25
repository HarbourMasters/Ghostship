// Packages the Ghostship mod SDK: the headers mods are built against, the mod tools and the template.
//   make_mod_sdk <ghostship checkout> <output folder>

#include <zip.h>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

const char* const kHeaderDirs[] = { "include", "src", "actors", "levels", "libultraship/include" };
const char* const kHeaderExts[] = { ".h", ".hpp", ".inc" };
const char* const kFiles[] = {
    "tools/merge.py",
    "tools/mod_manifest.py",
    "src/port/events/EVENTS.md",
    "properties.h",
};
const char* const kWorkspaces[] = { "workspace/template" };
const char* const kWorkspaceSkipDirs[] = { "build", "output", ".git", "__pycache__" };
const char* const kWorkspaceSkipExts[] = { ".o2r", ".zip" };

template <size_t N> bool Contains(const char* const (&list)[N], const std::string& value) {
    return std::find(std::begin(list), std::end(list), value) != std::end(list);
}

bool Version(const std::string& header, const char* name, int* out) {
    const std::regex pattern(std::string("#define\\s+") + name + "\\s+(\\d+)");
    std::smatch match;
    if (!std::regex_search(header, match, pattern)) {
        return false;
    }
    *out = std::stoi(match[1].str());
    return true;
}

void AddTree(const fs::path& repo, const std::string& base, bool workspace, std::set<std::string>& paths) {
    const fs::path root = repo / base;
    if (!fs::is_directory(root)) {
        return;
    }
    for (auto it = fs::recursive_directory_iterator(root); it != fs::recursive_directory_iterator(); ++it) {
        const std::string name = it->path().filename().string();
        if (it->is_directory()) {
            if (workspace && Contains(kWorkspaceSkipDirs, name)) {
                it.disable_recursion_pending();
            }
            continue;
        }
        if (!it->is_regular_file()) {
            continue;
        }
        const std::string ext = it->path().extension().string();
        if (workspace ? (Contains(kWorkspaceSkipExts, ext) || name == ".DS_Store") : !Contains(kHeaderExts, ext)) {
            continue;
        }
        paths.insert(fs::relative(it->path(), repo).generic_string());
    }
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: make_mod_sdk <ghostship checkout> <output folder>\n");
        return 1;
    }
    const fs::path repo = argv[1];
    const fs::path outDir = argv[2];

    std::ifstream api(repo / "src/port/api/api.h");
    std::stringstream header;
    header << api.rdbuf();
    int code;
    int apiVersion;
    if (!Version(header.str(), "GS_CODE_VERSION", &code) || !Version(header.str(), "GS_API_VERSION", &apiVersion)) {
        fprintf(stderr, "make_mod_sdk: GS_CODE_VERSION and GS_API_VERSION must be defined in src/port/api/api.h\n");
        return 1;
    }

    std::set<std::string> paths;
    for (const char* base : kHeaderDirs) {
        AddTree(repo, base, false, paths);
    }
    for (const char* file : kFiles) {
        if (fs::is_regular_file(repo / file)) {
            paths.insert(file);
        }
    }
    for (const char* base : kWorkspaces) {
        AddTree(repo, base, true, paths);
    }

    const std::string root = "ghostship-mod-sdk-code" + std::to_string(code) + "-api" + std::to_string(apiVersion);
    const fs::path out = outDir / (root + ".zip");
    fs::create_directories(outDir);
    fs::remove(out);

    int error = 0;
    zip_t* zip = zip_open(out.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (zip == nullptr) {
        fprintf(stderr, "make_mod_sdk: cannot create %s\n", out.string().c_str());
        return 1;
    }
    for (const std::string& path : paths) {
        zip_source_t* source = zip_source_file(zip, (repo / path).string().c_str(), 0, ZIP_LENGTH_TO_END);
        if (source == nullptr || zip_file_add(zip, (root + "/" + path).c_str(), source, ZIP_FL_ENC_UTF_8) < 0) {
            fprintf(stderr, "make_mod_sdk: cannot add %s: %s\n", path.c_str(), zip_strerror(zip));
            zip_source_free(source);
            zip_discard(zip);
            return 1;
        }
    }
    if (zip_close(zip) < 0) {
        fprintf(stderr, "make_mod_sdk: cannot write %s: %s\n", out.string().c_str(), zip_strerror(zip));
        zip_discard(zip);
        return 1;
    }
    printf("make_mod_sdk: wrote %s (%zu files)\n", out.string().c_str(), paths.size());
    return 0;
}
