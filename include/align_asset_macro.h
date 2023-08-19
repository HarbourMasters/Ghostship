#pragma once

#if defined(_WIN32)
    #define ALIGN_ASSET(x) __declspec(align(x))
#else
    #define ALIGN_ASSET(x) __attribute__((aligned (x)))
#endif