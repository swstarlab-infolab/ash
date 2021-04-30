#ifndef ASH_CONFIG_H
#define ASH_CONFIG_H
#include <ash/config/arch.h> // arch.h -> compiler.h -> osdetect.h

#if defined(ASH_ARCH_64)
#define ASH_ARCH_ALIGN (8)
#elif defined(ASH_ARCH_32)
#define ASH_ARCH_ALIGN (4)
#endif

#if defined(ASH_ENV_WINDOWS)
#define ASH_PATH_SEPARATOR '\\'
#define ASH_PATH_SEPARATOR_STR "\\"
#else
#define ASH_PATH_SEPARATOR '/'
#define ASH_PATH_SEPARATOR_STR "/"
#endif

#define ASH_DECLARE_SYMBOL(name) typedef struct name##__{}*name
#define ASH_DECLARE_HANDLE(name) typedef struct name##__{int i;}*name

namespace ash {

constexpr unsigned DiskSectorSize = 4096;
constexpr unsigned DefaultChannelSize = 8;

} // !namespace ash

#endif // ASH_CONFIG_H
