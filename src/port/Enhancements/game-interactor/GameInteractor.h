#ifdef __cplusplus
extern "C" {
#endif
#ifdef __cplusplus
}
#endif


#ifdef __cplusplus

#include <vector>
#include <functional>

#define DEFINE_HOOK(name, type)         \
    struct name {                       \
        typedef std::function<type> fn; \
    }

class GameInteractor {
public:
    static GameInteractor* Instance;

    // Game State
    class State {

    };

    // Game Hooks
    template <typename H> struct RegisteredGameHooks { inline static std::vector<typename H::fn> functions; };
    template <typename H> void RegisterGameHook(typename H::fn h) { RegisteredGameHooks<H>::functions.push_back(h); }
    template <typename H, typename... Args> void ExecuteHooks(Args&&... args) {
        for (auto& fn : RegisteredGameHooks<H>::functions) {
            fn(std::forward<Args>(args)...);
        }
    }

    DEFINE_HOOK(OnGameFrameUpdate, void());
    DEFINE_HOOK(OnHealthChange, void(int16_t health));

    // Helpers
    static bool IsSaveLoaded();
    static bool IsGameplayPaused();

    class RawAction {

    };

};

#endif // __cplusplus
