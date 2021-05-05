#include <ash/utility/dbg_log.h>
#include <ash/string.h>
#include <mutex>
#include <stdarg.h>
#include <stdio.h>


namespace ash {

void dbg_printf(char const* format, ...) {
    static char buffer[2048];
    static std::mutex mtx;
    std::lock_guard<std::mutex> guard(mtx);
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, 2048, format, ap);
    va_end(ap);
    printf("%s\n", buffer);
}

} // namespace ash