#ifndef ASH_POINTER_H
#define ASH_POINTER_H
#include <stdint.h>

namespace ash {

template <typename T, typename U>
T* seek_pointer(T* p, U offset) {
    static_assert(sizeof(void*) <= sizeof(uint64_t), "Pointer size is greater than 8!");
    auto const p2 = (char*)p;
    return reinterpret_cast<T*>(p2 + offset);
}

inline uint64_t ptrdiff(void* p1, void* p2) {
    static_assert(sizeof(void*) <= sizeof(uint64_t), "Pointer size is greater than 8!");
    uint64_t const _p1 = reinterpret_cast<uint64_t>(p1);
    uint64_t const _p2 = reinterpret_cast<uint64_t>(p2);
    return (_p1 > _p2) ? _p1 - _p2 : _p2 - _p1;
}

inline bool is_aligned_address(void* addr, uint64_t const alignment) {
    static_assert(sizeof(void*) <= sizeof(uint64_t), "Pointer size is greater than 8!");
    auto const addr2 = reinterpret_cast<uint64_t>(addr);
    return addr2 % alignment == 0;
}

inline bool is_aligned_address(void const* addr, uint64_t const alignment) {
    static_assert(sizeof(void*) <= sizeof(uint64_t), "Pointer size is greater than 8!");
    auto const addr2 = reinterpret_cast<uint64_t>(addr);
    return addr2 % alignment == 0;
}

template <typename T1, typename T2>
bool is_aligned_offset(T1 offset, T2 alignment) {
    return offset % alignment == 0;
}

} // !namespace ash
#endif // ASH_POINTER_H
