#include "EventSystem.h"
#include <stdexcept>
#include <algorithm>

#include "port/hooks/list/EngineEvent.h"

EventSystem* EventSystem::Instance = new EventSystem();

EventID EventSystem::RegisterEvent(const char* name) {
    const EventID id = this->mInternalEventID++;
    this->mEventRegistry[id] = EventRegistration{ .name = name == nullptr ? "Unknown" : name };
    return id;
}

ListenerID EventSystem::RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file, int line) {
    if (id == -1) {
        throw std::runtime_error("Trying to register listener for unregistered event");
    }

    auto& registry = this->mEventRegistry[id];

    if (std::find_if(registry.listeners.begin(), registry.listeners.end(),
                     [callback](const EventListener listener) { return listener.function == callback; }) != registry.listeners.end()) {
        throw std::runtime_error("Listener already registered");
    }

    registry.listeners.push_back({ priority, callback, { file, line, 0 } });

    // Sort by priority
    std::sort(registry.listeners.begin(), registry.listeners.end(),
              [](const EventListener a, const EventListener b) { return a.priority < b.priority; });

    return registry.listeners.size() - 1;
}

void EventSystem::UnregisterListener(EventID id, ListenerID listenerId) {
    auto& registry = this->mEventRegistry[id];

    registry.listeners.erase(registry.listeners.begin() + listenerId);
}

void EventSystem::CallEvent(const EventID id, IEvent* event, const char* file, const int line, const char* key) {
    auto& registry = this->mEventRegistry[id];

    for (auto&[priority, function, _] : registry.listeners) {
        function(event);
    }

    auto& info = registry.callers[key];

    if (info.path == nullptr) {
        info.path = file;
        info.line = line;
    }

    info.count++;
}

extern "C" EventID EventSystem_RegisterEvent(const char* name) {
    return EventSystem::Instance->RegisterEvent(name);
}

extern "C" ListenerID EventSystem_RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file, int line) {
    return EventSystem::Instance->RegisterListener(id, callback, priority, file, line);
}

extern "C" void EventSystem_UnregisterListener(EventID ev, ListenerID id) {
    EventSystem::Instance->UnregisterListener(ev, id);
}

extern "C" void EventSystem_CallEvent(EventID id, void* event, const char* file, int line, const char* key) {
    EventSystem::Instance->CallEvent(id, static_cast<IEvent*>(event), file, line, key);
}