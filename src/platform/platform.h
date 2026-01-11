#pragma once

// Platform detection
#if defined(_WIN32) || defined(_WIN64)
    #define PLATFORM_WINDOWS
#elif defined(__linux__)
    #define PLATFORM_LINUX
#elif defined(__APPLE__)
    #define PLATFORM_MACOS
#endif

// Platform-specific includes
#ifdef PLATFORM_WINDOWS
    #include <windows.h>
    #include <GL/wglew.h>
#endif

#ifdef PLATFORM_LINUX
    #include <EGL/egl.h>
    #include <CL/cl_egl.h>
    #include <SDL2/SDL_syswm.h>
#endif
