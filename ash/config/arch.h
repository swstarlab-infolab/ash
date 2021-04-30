#ifndef ASH_CONFIG_ARCH_H
#define ASH_CONFIG_ARCH_H
#include <ash/config/compiler.h>

/* Architecture */
#if defined(ASH_TOOLCHAIN_MSVC)
// ALPHA
#if defined(_M_ALPHA)
#define ASH_ARCH_ALPHA
#define ASH_ARCH_64
#endif // ALPHA
// AMD64
#if defined(_M_X64) || defined(_M_AMD64)
#define ASH_ARCH_AMD64
#define ASH_ARCH_64
#endif // AMD64
// ARM
#if defined(_M_ARM) || defined(_M_ARMT)
#define ASH_ARCH_ARM
#define ASH_ARCH_32
#endif // ARM
// Intel x86
#if defined(_M_I86) || defined(_M_IX86)
#define ASH_ARCH_X86
#define ASH_ARCH_32
#endif // Intel x86
// Intel Itanium (IA-64) 
#if defined(_M_IA64)
#define ASH_ARCH_IA64
#define ASH_ARCH_64
#endif // Intel Itanium (IA-64)
// PowerPC
#if defined(_M_PPC)
#define ASH_ARCH_PPC
#define ASH_ARCH_64
#endif // PowerPC

#elif defined(ASH_TOOLCHAIN_GNUC)
// ALPHA
#if defined(__alpha__)
#define ASH_ARCH_ALPHA
#define ASH_ARCH_64
#endif // ALPHA
// AMD64
#if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#define ASH_ARCH_AMD64
#define ASH_ARCH_64
#endif // AMD64
// ARM
#if defined(__arm__) || defined(__thumb__)
#define ASH_ARCH_ARM
#define ASH_ARCH_32
#endif // ARM
// ARM64
#if defined(__aarch64__)
#define ASH_ARCH_ARM64
#define ASH_ARCH_64
#endif // ARM64
// Intel x86
#if defined(i386) || defined(__i386) || defined(__i386__)
#define ASH_ARCH_X86
#define ASH_ARCH_32
#endif // Intel x86
// Intel Itanium (IA-64)
#if defined(__ia64__) || defined(_IA64) || defined(__IA64__)
#define ASH_ARCH_IA64
#define ASH_ARCH_64
#endif // Intel Itanium (IA-64)
// PowerPC
#if defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__ppc64__) || defined(__PPC__) || defined(__PPC64__) || defined(_ARCH_PPC) || defined(_ARCH_PPC64)
#define ASH_ARCH_PPC
#define ASH_ARCH_64
#endif // PowerPC

#elif defined(ASH_TOOLCHAIN_CLANG)
#error "TODO: Support the clang compiler"

#elif defined (ASH_TOOLCHAIN_IBMXL)
// Intel x86
#if defined(__THW_INTEL__)
#define ASH_ARCH_X86
#endif // Intel x86
// PowerPC
#if defined(_ARCH_PPC) || defined(_ARCH_PPC64)
#define ASH_ARCH_PPC
#define ASH_ARCH_64
#endif // PowerPC

#endif // Architecture

#endif // ASH_CONFIG_ARCH_H
