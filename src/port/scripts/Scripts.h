#pragma once

#include <stdint.h>

#ifdef __cplusplus
#include <deque>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Level and behavior scripts loaded from archives. See docs/romhacks/SCRIPTS.md.
namespace Scripts {

enum class Kind : uint8_t {
    Level = 0,
    Behavior = 1,
};

struct Value {
    enum class Type : uint8_t {
        Number = 0,
        Symbol = 1, // a behavior, function or level script by name, or an array of the same script
        Path = 2,   // an asset in the archives, e.g. "levels/bob/geo/bob_geo_000488"
    };
    Type type = Type::Number;
    int64_t number = 0;
    std::string text;
};

struct Command {
    std::string name;
    std::vector<Value> args;
    uint32_t line = 0;
};

struct Array {
    std::string name;
    std::vector<Command> commands;
};

struct Program {
    Kind kind = Kind::Level;
    std::vector<Array> arrays;
    // The array the script runs from. Empty picks the one named *_entry for level scripts, the first one otherwise.
    std::string entry;
};

struct Built {
    std::vector<std::vector<uintptr_t>> arrays;
    std::deque<std::string> paths; // the "__OTR__" strings commands point at
    size_t entry = 0;

    const uintptr_t* Entry() const {
        return arrays.empty() ? nullptr : arrays[entry].data();
    }
};

// Parses decomp-style `const LevelScript name[] = { ... };` blocks.
std::optional<Program> ParseText(Kind kind, std::string_view text, std::string& error);
bool Build(const Program& program, Built& out, std::string& error);

// Built in, registered, or scripts/behaviors/<name> from the archives. Null if not found.
const void* FindPointer(std::string_view name);
std::optional<int64_t> FindConstant(std::string_view name);
void RegisterPointer(std::string name, const void* value);
void RegisterConstant(std::string name, int64_t value);

} // namespace Scripts

extern "C" {
#endif

// For code mods: make a behavior, function or level script usable by name in archived scripts.
void Scripts_RegisterPointer(const char* name, const void* value);
void Scripts_RegisterConstant(const char* name, int64_t value);

#ifdef __cplusplus
}
#endif
