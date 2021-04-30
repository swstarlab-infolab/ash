#ifndef ASH_CONFIG_COMPILER_H
#define ASH_CONFIG_COMPILER_H
#include <ash/config/osdetect.h>
#include <ash/pp/pparg.h>

/* Toolchain */
#if defined(_MSC_VER)
#define ASH_TOOLCHAIN_MSVC
#define ASH_TOOLCHAIN_VER _MSC_FULL_VER
#define ASH_TOOLCHAIN_NAME "msvc" ASH_PP_STRING(ASH_TOOLCHAIN_VER)

#elif defined(__GNUC__) && !defined(__clang__)
#define ASH_TOOLCHAIN_GNUC
#define ASH_TOOLCHAIN_VER (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define ASH_TOOLCHAIN_NAME "gcc " ASH_PP_STRING(ASH_TOOLCHAIN_VER)
#define fopen_s(fp, fmt, mode)          *(fp)=fopen( (fmt), (mode))
#define strcpy_s(dst, max_len, src)		strncpy((dst), (src), (max_len))

#elif defined(__clang__) && !defined(__ibmxl__)
#define ASH_TOOLCHAIN_CLANG
#define ASH_TOOLCHAIN_VER (clang_major * 10000 + clang_minor * 100 + __clang_patchlevel__)
#define ASH_TOOLCHAIN_NAME "clang " ASH_PP_STRING(ASH_TOOLCHAIN_VER)
#define fopen_s(fp, fmt, mode)          *(fp)=fopen( (fmt), (mode))
#define strcpy_s(dst, max_len, src)		strncpy((dst), (src), (max_len))

#elif defined(__ibmxl__)
#define ASH_TOOLCHAIN_IBMXL
#define ASH_TOOLCHAIN_VER __ibmxl_version__
#define ASH_TOOLCHAIN_NAME "ibmxl" ASH_PP_STRING(ASH_TOOLCHAIN_VER)
#define fopen_s(fp, fmt, mode)          *(fp)=fopen( (fmt), (mode))
#define strcpy_s(dst, max_len, src)		strncpy((dst), (src), (max_len))

#elif defined(__CUDACC__)
#define ASH_TOOLCHAIN_NVCC
#define ASH_TOOLCHAIN_VER __CUDACC_VER__
#define ASH_TOOLCHAIN_NAME "nvcc" ASH_PP_STRING(ASH_TOOLCHAIN_VER)

#elif defined(__INTEL_COMPILER)
#define ASH_TOOLCHAIN_INTEL
#define ASH_TOOLCHAIN_VER __INTEL_COMPILER
#define ASH_TOOLCHAIN_NAME "intel" ASH_PP_STRING(ASH_TOOLCHAIN_VER)
#define fopen_s(fp, fmt, mode)          *(fp)=fopen( (fmt), (mode))
#define strcpy_s(dst, max_len, src)		strncpy((dst), (src), (max_len))

#else

#error "Unrecognized toolchain"

#endif // Toolchain

#define ASH_PTR_SIZE sizeof(void*)
#define _ash_unused(x) ((void)(x))

#if defined(ASH_TOOLCHAIN_MSVC)
#define ASH_LIKELY(expression) (expression)
#define ASH_UNLIKELY(expression) (expression)
#define ASH_NOINLINE __declspec(noinline)
#define ASH_FORCEINLINE __forceinline
#define ASH_RESTRICT __restrict
#define ASH_ALIGN(n) __declspec(align(n))
#endif

#if defined(ASH_TOOLCHAIN_GNUC)
#define ASH_LIKELY(expression) __builtin_expect(expression, 1)
#define ASH_UNLIKELY(expression) __builtin_expect(expression, 0)
#define ASH_NOINLINE __attribute__((noinline))
#define ASH_FORCEINLINE __inline __attribute__((always_inline))
#define ASH_RESTRICT __restrict__
#define ASH_ALIGN(n) __attribute__((aligned(n)))
#   if defined(ASH_ENV_WINDOWS)
#   define ASH_SYMBOL_EXPORT __attribute__((__dllexport__))
#   define ASH_SYMBOL_IMPORT __attribute__((__dllimport__))
#   else
#   define ASH_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#   define ASH_SYMBOL_IMPORT
#   endif
#define ASH_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#endif

#if defined(ASH_TOOLCHAIN_CLANG)
#define ASH_LIKELY(expression) __builtin_expect(expression, 1)
#define ASH_UNLIKELY(expression) __builtin_expect(expression, 0)
#define ASH_NOINLINE __attribute__((noinline))
#define ASH_FORCEINLINE inline
#define ASH_RESTRICT restrict
#define ASH_ALIGN(n) __attribute__((aligned(n)))
#   if defined(ASH_ENV_WINDOWS)
#   define ASH_SYMBOL_EXPORT __attribute__((__dllexport__))
#   define ASH_SYMBOL_IMPORT __attribute__((__dllimport__))
#   else
#   define ASH_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#   define ASH_SYMBOL_IMPORT
#   endif
#define ASH_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#endif

#if defined(ASH_TOOLCHAIN_IBMXL)
#define ASH_LIKELY(expression) __builtin_expect(expression, 1)
#define ASH_UNLIKELY(expression) __builtin_expect(expression, 0)
#define ASH_NOINLINE 
#define ASH_FORCEINLINE inline
#define ASH_RESTRICT __restrict__
#define ASH_ALIGN(n) 
#define ASH_NO_ALIGNMENT
#define ASH_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#define ASH_SYMBOL_IMPORT
#define ASH_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#endif

#if defined(ASH_TOOLCHAIN_NVCC)
#define ASH_LIKELY(expression) 
#define ASH_UNLIKELY(expression) 
#define ASH_NOINLINE __noinline__
#define ASH_FORCEINLINE __forceinline__
#define ASH_RESTRICT __restrict__
#define ASH_ALIGN(n) __align__(n)
#define ASH_NO_ALIGNMENT
#endif

#if defined(ASH_TOOLCHAIN_INTEL)
#define ASH_LIKELY(expression) __builtin_expect(expression, 1)
#define ASH_UNLIKELY(expression) __builtin_expect(expression, 0)
#define ASH_NOINLINE 
#define ASH_FORCEINLINE inline
#define ASH_RESTRICT restrict
#define ASH_ALIGN(n) 
#define ASH_NO_ALIGNMENT
#define ASH_SYMBOL_EXPORT __attribute__((visibility("default")))
#define ASH_SYMBOL_IMPORT
#define ASH_SYMBOL_VISIBLE __attribute__((visibility("default")))
#endif

#endif // ASH_CONFIG_COMPILER_H
