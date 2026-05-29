#pragma once
#ifndef __SWITCH__

#include <functional>
#include <string>

namespace Permissions {

enum class State { Pending, Allowed, Denied };

struct PromptDef {
    std::string title;    // dialog title bar
    std::string message;  // body text shown to the user
};

// Register a permission key and its associated prompt text.
// Idempotent: calling again with the same key updates the prompt.
void Register(const std::string& key, PromptDef prompt);

// Returns the current persisted state for `key`.
// Returns Pending for unknown keys.
State Get(const std::string& key);

// Shows the permission dialog if state is Pending.
// `onAllow` and `onDeny` are invoked after the user clicks a button.
// Both may be nullptr.
void Request(const std::string& key,
             std::function<void()> onAllow = nullptr,
             std::function<void()> onDeny  = nullptr);

} // namespace Permissions
#endif
