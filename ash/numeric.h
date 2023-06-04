#ifndef ASH_NUMERIC_H
#define ASH_NUMERIC_H
#include <ash/config.h>
#include <type_traits>
#include <stdint.h>
#include <stddef.h>

namespace ash {

template <typename T = size_t>
ASH_FORCEINLINE constexpr T KiB(size_t const i) {
    return i << 10;
}

template <typename T = size_t>
ASH_FORCEINLINE constexpr T MiB(size_t const i) {
    return i << 20;
}

template <typename T = size_t>
ASH_FORCEINLINE constexpr T GiB(size_t const i) {
    return (T)(i << 30);
}

template <typename T = size_t>
ASH_FORCEINLINE constexpr T TiB(size_t const i) {
    return i << 40;
}

template <typename T1, typename T2>
ASH_FORCEINLINE constexpr T1 aligned_size(const T1 size, const T2 align) {
    return align * ((size + align - 1) / align);
}

template <typename T1, typename T2>
ASH_FORCEINLINE constexpr T1 padding_size(T1 const size, T2 const align) {
    return aligned_size(size, align) - size;
}

template <typename T>
ASH_FORCEINLINE constexpr bool is_power_of_two(T value) {
    static_assert(std::is_integral<T>::value, "invalid argument! type must be integral type");
    return value && ((value & (value - 1)) == 0);
}

template <size_t S>
struct naturally_aliged_size {
    constexpr static size_t value = aligned_size(S, ASH_ARCH_ALIGN);
};

#define ASH_NATURALLY_ALIGNED_SIZE(S, Value) \
template <>\
struct naturally_aliged_size<S> {\
    constexpr static size_t value = Value;\
};

ASH_NATURALLY_ALIGNED_SIZE(0, 0);
ASH_NATURALLY_ALIGNED_SIZE(1, 1);
ASH_NATURALLY_ALIGNED_SIZE(2, 2);
ASH_NATURALLY_ALIGNED_SIZE(3, 4);
ASH_NATURALLY_ALIGNED_SIZE(4, 4);
ASH_NATURALLY_ALIGNED_SIZE(5, 8);
ASH_NATURALLY_ALIGNED_SIZE(6, 8);
ASH_NATURALLY_ALIGNED_SIZE(7, 8);
ASH_NATURALLY_ALIGNED_SIZE(8, 8);

#undef ASH_NATURALLY_ALIGNED_SIZE

template <typename T>
ASH_FORCEINLINE typename std::enable_if<std::is_signed<T>::value, T>::type roundup(const T n, const T m) {
    return ((n + ((n >= 0) ? 1 : 0) * (m - 1)) / m) * m;
}

template <typename T>
ASH_FORCEINLINE typename std::enable_if<std::is_unsigned<T>::value, T>::type roundup(const T n, const T m) {
    return ((n + m - 1) / m) * m;
}

ASH_FORCEINLINE uint32_t roundup2_nonzero(uint32_t n) {
    --n;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    ++n;
    return n;
}

ASH_FORCEINLINE uint32_t roundup2(uint32_t n) {
    n += (n == 0);
    return roundup2_nonzero(n);
}

ASH_FORCEINLINE uint64_t roundup2_nonzero(uint64_t n) {
    --n;
    n |= (n >> 1);
    n |= (n >> 2);
    n |= (n >> 4);
    n |= (n >> 8);
    n |= (n >> 16);
    n |= (n >> 32);
    ++n;
    return n;
}

ASH_FORCEINLINE uint64_t roundup2(uint64_t n) {
    n += (n == 0);
    return roundup2_nonzero(n);
}

template <uint32_t Lv, uint32_t V, uint32_t R, uint32_t ...Rest>
struct _static_log2_impl;

template <uint32_t Lv, uint32_t V, uint32_t R, uint32_t C, uint32_t ...Rest>
struct _static_log2_impl<Lv, V, R, C, Rest...> {
private:
    static constexpr uint32_t Shift = (V > C) << Lv;
public:
    static constexpr uint32_t value = _static_log2_impl<Lv - 1, (V >> Shift), (R | Shift), Rest ... > ::value;
};

template <uint32_t V, uint32_t R, uint32_t C>
struct _static_log2_impl<1, V, R, C> {
private:
    static constexpr uint32_t Shift = (V > C) << 1;
public:
    static constexpr uint32_t value = _static_log2_impl<0, (V >> Shift), (R | Shift)>::value;
};

template <uint32_t V, uint32_t R>
struct _static_log2_impl<0, V, R> {
    static constexpr uint32_t value = R | (V >> 1);
};

template <uint32_t V>
struct static_log2 {
    static_assert(V != 0, "The logarithm of zero is undefined");
private:
    static constexpr uint32_t R = (V > 0xFFFF);
public:
    static constexpr uint32_t value =
        _static_log2_impl<3, (V >> R), R, 0xFF, 0xF, 0x3>::value;
};

inline unsigned log2u(unsigned v) {
    // source: https://graphics.stanford.edu/~seander/bithacks.html
    static unsigned const b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000 };
    static unsigned const s[] = { 1, 2, 4, 8, 16 };
    unsigned int r = 0;
    for (auto i = 4; i >= 0; i--) {
        if (v & b[i]) {
            v >>= s[i];
            r |= s[i];
        }
    }
    return r;
}

inline uint64_t log2u(uint64_t v) {
    static uint64_t const b[] = { 0x2, 0xC, 0xF0, 0xFF00, 0xFFFF0000, 0xFFFFFFFF00000000 };
    static uint64_t const s[] = { 1, 2, 4, 8, 16, 32 };
    uint64_t r = 0;
    for (auto i = 5; i >= 0; i--) {
        if (v & b[i]) {
            v >>= s[i];
            r |= s[i];
        }
    }
    return r;
}

inline unsigned log2u_nb(unsigned v) {
    unsigned r = (v > 0xFFFF) << 4; v >>= r;
    unsigned shift = (v > 0xFF) << 3; v >>= shift; r |= shift;
    shift = (v > 0xF) << 2; v >>= shift; r |= shift;
    shift = (v > 0x3) << 1; v >>= shift; r |= shift;
    r |= (v >> 1);
    return r;
}

template <typename T>
struct closed_interval {
    using value_type = T;
    value_type lo;
    value_type hi;
    value_type width() const {
        return hi - lo + 1;
    }
};

} // !namespace ash
#endif // ASH_NUMERIC_H
