#ifndef ASH_CONFIG_OSDETECT_H
#define ASH_CONFIG_OSDETECT_H

#if defined(__gnu_linux__)
#define ASH_ENV_GNU_LINUX
#endif

#if defined(_WIN32)
#define ASH_ENV_WINDOWS
#   if defined(_WIN64)
#   define ASH_ENV_WIN64
#   else
#   define ASH_ENV_WIN32
#   endif
#endif

#if defined(__unix__) || defined(__unix)
#define ASH_ENV_UNIX
#endif

#if defined(__linux__)
#define ASH_ENV_LINUX
#endif

#if defined(macintosh) || (defined(__APPLE__) && defined(__MACH__))
#define ASH_ENV_MACOS
#endif

#endif // ASH_CONFIG_OSDETECT_H
