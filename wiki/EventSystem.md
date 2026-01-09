# Guide: Implementing New Events in the Event System

This guide explains how to define, register, call, and listen to new events, based on the `RenderPauseCourseOptions` implementation.

---

## 1. Define the Event Structure
Events must be defined in a header file so the system knows what data they carry.

**File:** `src/port/hooks/list/EngineEvent.h`

Use the `DEFINE_EVENT` macro. If the event needs to pass data to listeners (like a pointer to a boolean), define those fields inside the macro.

```cpp
DEFINE_EVENT(RenderPauseCourseOptions,
    bool* render; // Data passed to listeners to modify
);
```

---

## 2. Register the Event
The system must register the event ID during the engine's registration phase.

**File:** `src/port/mods/PortEnhancements.c`

Add your event to the `PortEnhancements_Register` function:

```c
void PortEnhancements_Register() {
    // ...
    REGISTER_EVENT(RenderPauseCourseOptions);
}
```

---

## 3. Call the Event in Source Code
Trigger the event in the game logic where you want the hook to occur.

**File:** `src/game/ingame_menu.c`

Use `CALL_CANCELLABLE_EVENT`. This fires the event and allows listeners to modify data or cancel the execution of the code block following it.

```c
// Prepare the data
bool canPause = gMarioStates[0].action & ACT_FLAG_PAUSE_EXIT;

// Trigger the event
CALL_CANCELLABLE_EVENT(RenderPauseCourseOptions, &canPause) {
    // This block only runs if no listener cancels the event
    if (canPause) {
        render_pause_course_options(99, 93, &gDialogLineNum, 15);
    }
}
```

---

## 4. Create the Listener (Callback)
Implement the logic that reacts to the event. This is usually where you check for CVars or custom mod logic.

**File:** `src/port/mods/PortEnhancements.c`

```c
void OnRenderPauseCourseOptions(IEvent* event) {
    // Only apply the logic if the specific CVar is enabled
    if (CVarGetInteger("gPauseExitWhenever", 0) == 0) {
        return;
    }

    // Cast the generic IEvent to your specific event type
    RenderPauseCourseOptions* ev = (RenderPauseCourseOptions*)event;
    
    // Modify the passed data
    *ev->render = true;
}
```

---

## 5. Register the Listener
Finally, tell the system to execute your callback when the event is triggered.

**File:** `src/port/mods/PortEnhancements.c`

Inside `PortEnhancements_Init`, use `REGISTER_LISTENER`:

```c
void PortEnhancements_Init() {
    // ...
    REGISTER_LISTENER(RenderPauseCourseOptions, OnRenderPauseCourseOptions, EVENT_PRIORITY_NORMAL);
}
```

---

## Summary of Macros
| Macro | Purpose |
| :--- | :--- |
| **DEFINE_EVENT** | Defines the event name and its internal data structure. |
| **REGISTER_EVENT** | Informs the system of the event's existence during startup. |
| **CALL_CANCELLABLE_EVENT** | Executes the event and a conditional code block. |
| **REGISTER_LISTENER** | Connects a specific function to an event trigger. |