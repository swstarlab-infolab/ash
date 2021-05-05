#ifndef ASH_UTILITY_DBG_LOG_H
#define ASH_UTILITY_DBG_LOG_H
#include <stdlib.h>

namespace ash {

//TODO: ADD TIME-STAMP BASED LOG STRUCTURE AND THREAD-SAFE GLOBAL LOGGING APIS

void dbg_printf(char const* format, ...);
void dbg_message(char const* prefix, char const* file, int line, char const* format, ...);
void dbg_println(char const* format, ...);

#define ASH_DMESG(MESSAGE, ...) ::ash::dbg_message("log", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__)
#define ASH_FATAL(MESSAGE, ...) do { ::ash::dbg_message("FATAL", __FILE__, __LINE__, MESSAGE, ##__VA_ARGS__); exit(-1); } while(0) 

} // namespace ash
#endif // ASH_UTILITY_DBG_LOG_H
