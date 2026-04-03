#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef int32_t EventID;
typedef uint32_t ListenerID;

typedef enum {
    EVENT_PRIORITY_LOW,
    EVENT_PRIORITY_NORMAL,
    EVENT_PRIORITY_HIGH,
} EventPriority;

typedef struct {
    bool cancelled;
} IEvent;

typedef void (*EventCallback)(IEvent*);

typedef struct EventMetadata {
    const char* path;
    int line;
    uint64_t count;
} EventMetadata;

typedef struct EventListener {
    EventPriority priority;
    EventCallback function;
    EventMetadata metadata;
} EventListener;

#ifndef __cplusplus
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) \
    uint32_t eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) \
    extern uint32_t eventName##ID;
#endif
#else
#ifdef INIT_EVENT_IDS
#define DECLARE_EVENT(eventName) \
    extern "C" uint32_t eventName##ID = -1;
#else
#define DECLARE_EVENT(eventName) \
    extern "C" uint32_t eventName##ID;
#endif
#endif

#define STRINGIFY_DETAIL(x) #x 
#define TOSTRING(x) STRINGIFY_DETAIL(x)

#define FILE_AND_LINE __FILE__ ":" TOSTRING(__LINE__)

#define DEFINE_EVENT(eventName, ...) \
    typedef struct { \
        IEvent event; \
        __VA_ARGS__ \
    } eventName; \
    \
    DECLARE_EVENT(eventName)

#define CALL_EVENT(eventType, ...) \
    eventType eventType##_ = { {false}, __VA_ARGS__ }; \
    EventSystem_CallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE);

#define CALL_CANCELLABLE_EVENT(eventType, ...) \
    eventType eventType##_ = { {false}, __VA_ARGS__ }; \
    EventSystem_CallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (!eventType##_.event.cancelled)

#define CHECK_IF_NOT_CANCELLED(eventType) \
    if (!eventType##_.event.cancelled)

#define CALL_CANCELLABLE_RETURN_EVENT(eventType, ...) \
    eventType eventType##_ = { {false}, __VA_ARGS__ }; \
    EventSystem_CallEvent(eventType##ID, &eventType##_, __FILE__, __LINE__, FILE_AND_LINE); \
    if (eventType##_.event.cancelled) { \
        return; \
    }

#define REGISTER_EVENT(eventType) \
    eventType##ID = EventSystem_RegisterEvent(#eventType);

#define REGISTER_LISTENER(eventType, priority, callback) \
    EventSystem_RegisterListener(eventType##ID, callback, priority, __FILE__, __LINE__);

#define UNREGISTER_LISTENER(eventType, listenerID) \
    EventSystem_UnregisterListener(eventType##ID, listenerID);

#ifdef __cplusplus
#include <vector>
#include <unordered_map>

struct EventRegistration {
    const char* name;
    std::unordered_map<const char*, EventMetadata> callers;
    std::vector<EventListener> listeners;
};

class EventSystem {
public:
    static EventSystem* Instance;
    EventID RegisterEvent(const char* name = nullptr);
    ListenerID RegisterListener(EventID id, EventCallback callback, EventPriority priority = EVENT_PRIORITY_NORMAL, const char* file = nullptr, int line = 0);
    void UnregisterListener(EventID ev, ListenerID id);
    void CallEvent(EventID id, IEvent* event, const char* file = nullptr, int line = 0, const char* key = nullptr);

    EventRegistration* GetEventRegistration(EventID id) {
        if (mEventRegistry.contains(id)) {
            return &mEventRegistry.at(id);
        }
        return nullptr;
    }

    std::unordered_map<EventID, EventRegistration>& GetEventRegistrations() {
        return this->mEventRegistry;
    }
private:
    std::unordered_map<EventID, EventRegistration> mEventRegistry;
    EventID mInternalEventID = 0;
};

extern "C" {
#endif
extern EventID EventSystem_RegisterEvent(const char* name);
extern ListenerID EventSystem_RegisterListener(EventID id, EventCallback callback, EventPriority priority, const char* file, int line);
extern void EventSystem_UnregisterListener(EventID ev, ListenerID id);
extern void EventSystem_CallEvent(EventID id, void* event, const char* file, int line, const char* key);
#ifdef __cplusplus
}
#endif