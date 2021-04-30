#ifndef ASH_MALLOC_H
#define ASH_MALLOC_H
#include <ash/config.h>
#include <utility>
#include <malloc.h>

namespace ash {

template <typename T, typename ...Args>
ASH_FORCEINLINE void placement_new(T* p, Args&& ...args) {
    new (p) T(std::forward<Args>(args)...);
}

template <typename T>
ASH_FORCEINLINE void placement_delete(T* p) {
    p->~T();
}

#if defined(ASH_TOOLCHAIN_MSVC)

inline void* aligned_malloc(size_t size, size_t alignment) {
    return _aligned_malloc(size, alignment);
}

inline void aligned_free(void* p) {
    _aligned_free(p);
}

#else 

inline void* aligned_malloc(size_t size, size_t alignment) {
    void* p;
    p = memalign(alignment, size);
    return p;
    //    if (__builtin_expect(0 == posix_memalign(&p, alignment, size), true))
    //        return p;
    //    return nullptr;
}

inline void aligned_free(void* p) {
    free(p);
}

#endif

} // !namespace ash
#endif // ASH_MALLOC_H
