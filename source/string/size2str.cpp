#include <ash/string.h>
#include <stdio.h>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

namespace ash {

static const char* size_defs_binary[] = { " EiB", " PiB", " TiB", " GiB", " MiB", " KiB", "-byte" };
static const char* size_defs_decimal[] = { " E", " P", " T", " G", " M", " K", "" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
static const uint64_t  exabyte = 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;

char* size2str_decimal(uint64_t size, char* out, size_t bufsize) {
    constexpr int dim = sizeof(size_defs_decimal) / sizeof(*size_defs_decimal);
    if (size == 0) {
        snprintf(out, bufsize, "0%s", size_defs_binary[dim - 1]);
    }
    uint64_t  multiplier = exabyte;
    for (size_t i = 0; i < dim; i++, multiplier /= 1000) {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            snprintf(out, bufsize, "%" PRIu64 "%s", size / multiplier, size_defs_decimal[i]);
        else
            snprintf(out, bufsize, "%.1f%s", static_cast<float>(size) / static_cast<float>(multiplier), size_defs_decimal[i]);
        return out;
    }
    return nullptr;
}

char* size2str_binary(uint64_t size, char* out, size_t bufsize) {
    constexpr int dim = sizeof(size_defs_binary) / sizeof(*size_defs_binary);
    if (size == 0) {
        snprintf(out, bufsize, "0%s", size_defs_binary[dim - 1]);
    }
    uint64_t  multiplier = exbibytes;
    for (size_t i = 0; i < dim; i++, multiplier /= 1024) {
        if (size < multiplier)
            continue;
        if (size % multiplier == 0)
            snprintf(out, bufsize, "%" PRIu64 "%s", size / multiplier, size_defs_binary[i]);
        else
            snprintf(out, bufsize, "%.1f%s", static_cast<float>(size) / static_cast<float>(multiplier), size_defs_binary[i]);
        return out;
    }
    return nullptr;
}

char* size2str_binary(uint64_t size, size_prefix base, char* out, size_t bufsize) {
    uint64_t  multiplier = exbibytes;
    constexpr int dim = sizeof(size_defs_binary) / sizeof(*size_defs_binary);
    if (size == 0) {
        snprintf(out, bufsize, "0%s", size_defs_binary[dim - 1]);
    }
    for (int i = 0; i < static_cast<int>(base); ++i)
        multiplier /= 1024;
    if (size % multiplier == 0)
        snprintf(out, bufsize, "%" PRIu64 "%s", size / multiplier, size_defs_binary[static_cast<int>(base)]);
    else
        snprintf(out, bufsize, "%.1f%s", static_cast<float>(size) / multiplier, size_defs_binary[static_cast<int>(base)]);
    return out;
}

} // !namespace ash