#if defined(__WINRT__)

#define SDL_MAIN_HANDLED

#include <windows.h>
#include <roapi.h>

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#include <SDL2/SDL_main.h>
#else
#include <SDL.h>
#include <SDL_main.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

extern "C" int SDL_main(int argc, char** argv);

static void UwpRedirectStandardStreams() {
    const wchar_t* local = SDL_WinRTGetFSPathUNICODE(SDL_WINRT_PATH_LOCAL_FOLDER);
    if (local == nullptr) {
        return;
    }

    auto redirect = [local](const wchar_t* leaf, FILE* stream) {
        wchar_t path[512];
        size_t i = 0;
        for (; local[i] != L'\0' && i < 400; ++i) {
            path[i] = local[i];
        }
        path[i++] = L'\\';
        for (size_t j = 0; leaf[j] != L'\0' && i < 510; ++j, ++i) {
            path[i] = leaf[j];
        }
        path[i] = L'\0';

        FILE* reopened = nullptr;
        _wfreopen_s(&reopened, path, L"w", stream);
    };

    redirect(L"stdout.txt", stdout);
    redirect(L"stderr.txt", stderr);
}

// Not empty by accident: installing any handler stops the UCRT calling __fastfail on a bad
// CRT argument, which kills an app container outright with nothing logged.
static void UwpInvalidParameter(const wchar_t* expression, const wchar_t* function, const wchar_t* file,
                                unsigned int line, uintptr_t /*reserved*/) {
    (void)expression;
    (void)function;
    (void)file;
    (void)line;
}

int main(int /*argc*/, char** /*argv*/) {
    const HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
    if (FAILED(hr) && hr != S_FALSE && hr != RPC_E_CHANGED_MODE) {
        return 1;
    }

    _set_invalid_parameter_handler(UwpInvalidParameter);
    UwpRedirectStandardStreams();

    SDL_SetHint(SDL_HINT_WINRT_HANDLE_BACK_BUTTON, "1");

    const int rc = SDL_WinRTRunApp(SDL_main, nullptr);

    return rc;
}

#endif
