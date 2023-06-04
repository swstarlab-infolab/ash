#ifndef ASH_UTILITY_DBG_LOG_H
#define ASH_UTILITY_DBG_LOG_H
#include <stdlib.h>
#include <stdio.h>
#include <stdexcept>
#include <ash/pp/pparg.h>

namespace ash {

//TODO: ADD TIME-STAMP BASED LOG STRUCTURE AND THREAD-SAFE GLOBAL LOGGING APIS

void dbg_printf(char const* format, ...);
void dbg_message(char const* prefix, char const* file, int line, char const* format, ...);
void dbg_println(char const* format, ...);

#ifdef ASH_IGNORE_DMESG
#define ASH_DMESG(...)
#else
#define ASH_DMESG(MESSAGE, ...) ::ash::dbg_message("dmesg", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__)
#endif
#define ASH_LOG(MESSAGE, ...) ::ash::dbg_message("log", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__)
#define ASH_ERRLOG(MESSAGE, ...) ::ash::dbg_message("err", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__)

#define ASH_FATAL(MESSAGE, ...) do { ::ash::dbg_message("FATAL", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__); fflush(stdout); assert(false); exit(-1); } while(0) 
#define ASH_FATAL_ASSERT(COND, MESSAGE, ...) if (!(COND)) { ASH_FATAL(MESSAGE, __VA_ARGS__); }

char const* __ash_what(const std::exception_ptr& eptr = std::current_exception());

#define ASH_CLAUSE_CATCH_STDEXCEPT \
    catch (...) {\
        std::exception_ptr eptr = std::current_exception();\
        ASH_ERRLOG("Caught unhandled exception \"%s\"", ::ash::__ash_what(eptr));\
    }

#define ASH_CLAUSE_CATCH_STDEXCEPT_WITH(Expr) \
    catch (...) {\
        std::exception_ptr eptr = std::current_exception();\
        ASH_ERRLOG("Caught unhandled exception \"%s\"", ::ash::__ash_what(eptr));\
        Expr;\
    }

} // namespace ash
#endif // ASH_UTILITY_DBG_LOG_H
