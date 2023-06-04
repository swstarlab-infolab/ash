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
    fflush(stdout);
}

void dbg_message(char const* prefix, char const* file, int const line, char const* format, ...) {
    static char buffer[2048];
    static std::mutex mtx;
    std::lock_guard<std::mutex> guard(mtx);
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, 2048, format, ap);
    va_end(ap);
    printf("%s > %s:%d\n", prefix, basename(file, ASH_PATH_SEPARATOR), line);
    printf("%s\n", buffer);
    fflush(stdout);
}

void dbg_println(char const* format, ...) {
    static char buffer[2048];
    static std::mutex mtx;
    std::lock_guard<std::mutex> guard(mtx);
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, 2048, format, ap);
    va_end(ap);
    printf("%s\n", buffer);
    fflush(stdout);
}

char const* __ash_what(const std::exception_ptr& eptr) {
    try {
        if (eptr)
            std::rethrow_exception(eptr);
    }
    catch (const std::exception& e) {
        return e.what();
    }
    catch (const std::string& e) {
        return e.c_str();
    }
    catch (const char* e) {
        return e;
    }
    catch (...) {
        return "null (unknown)";
    }
    return "null exception";
}

} // namespace ash
