#pragma once

#include <stdbool.h>
#include <stdint.h>

// Port features a romhack or mod can turn off (disable) or pin to a value (force). The rules come from the active
// romhack and each mounted mod's manifest.json (port/ModRules.h) and are fixed for the session; nothing writes CVars,
// so the player's own value comes back once the rule is gone. Code reads a lockable setting through Port_Feature*
// instead of CVarGet*. See docs/romhacks/DESIGN.md#feature-rules.

#ifdef __cplusplus
// Game headers that include this one are often included inside extern "C" blocks.
extern "C++" {
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace Features {

// One romhack or mod and its rules. disable holds feature ids or group.* wildcards; force maps them to values.
struct RuleSource {
    std::string name;
    std::vector<std::string> disable;
    std::vector<std::pair<std::string, nlohmann::json>> force;
};

// Two sources forcing different values; the later one in load order wins.
struct Conflict {
    std::string id;
    std::string winner;
    std::string loser;
};

// Applies the rules of every source, in load order. Replaces the rules applied before.
void Resolve(const std::vector<RuleSource>& sources);

bool IsLocked(const char* id);
// "Disabled by X" or "Set to V by X", empty when unlocked.
std::string LockReason(const char* id);
// The feature id a widget's CVar belongs to, or NULL.
const char* ForCVar(const char* cvar);
const std::vector<Conflict>& Conflicts();
// For windows a feature covers: draws the lock reason and returns true while the feature is disabled.
bool DrawLockedNotice(const char* id);

} // namespace Features
}

extern "C" {
#endif

// FALSE while any source disables the feature. For actions and windows.
bool Port_FeatureAllowed(const char* id);
// The forced value, else the feature's vanilla value while disabled, else its CVar (fallback is the CVar default).
int32_t Port_FeatureInt(const char* id, int32_t fallback);
float Port_FeatureFloat(const char* id, float fallback);
// Name of the first source that disables the feature, or NULL.
const char* Port_FeatureDisabledBy(const char* id);

#ifdef __cplusplus
}
#endif
