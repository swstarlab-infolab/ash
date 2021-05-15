#include <ash/string.h>
#include <stdio.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace ash {

static const char* size_defs_binary[] = { " EiB", " PiB", " TiB", " GiB", " MiB", " KiB", "-byte" };
static const char* size_defs_decimal[] = { " E", " P", " T", " G", " M", " K", "" };
static const uint64_t  exbibytes = 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL * 1024ULL;
static const uint64_t  exabyte = 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL * 1000ULL;

} // !namespace ash