#ifndef ASH_UTILITY_STD_ATOMIC_CONATINER_API_H
#define ASH_UTILITY_STD_ATOMIC_CONATINER_API_H
#include <atomic>

namespace ash {

template <typename T>
void _cpy_atomic_arr_to_plain_arr(T* dst, std::atomic<T>* src, size_t count, std::true_type) {
    memcpy((void*)dst, src, sizeof(T) * count);
}

template <typename T>
void _cpy_atomic_arr_to_plain_arr(T* dst, std::atomic<T>* src, size_t count, std::false_type) {
    for (size_t i = 0; i < count; ++i) {
        dst[i] = src[i].load(std::memory_order_relaxed);
    }
}

template <typename T>
void cpy_atomic_arr_to_plain_arr(T* dst, std::atomic<T>* src, size_t count) {
    _cpy_atomic_arr_to_plain_arr(dst, src, count, std::integral_constant<bool, sizeof(T) == sizeof(std::atomic<T>)>{});
}

template <typename T>
void _zerofill_atomic_arr(std::atomic<T>* arr, size_t count, std::true_type) {
    memset(arr, 0, sizeof(T) * count);
}


template <typename T>
void _zerofill_atomic_arr(std::atomic<T>* arr, size_t count, std::false_type) {
    for (size_t i = 0; i < count; ++i) {
        arr[i].store(0, std::memory_order_relaxed);
    }
}

template <typename T>
void zerofill_atomic_arr(std::atomic<T>* arr, size_t count) {
    _zerofill_atomic_arr(arr, count, std::integral_constant<bool, sizeof(T) == sizeof(std::atomic<T>)>{});
}

template<typename T>
void atomic_update_maximum(std::atomic<T>& maximum_value, T const& value) noexcept {
    T expected = maximum_value;
    while (expected < value &&
        !maximum_value.compare_exchange_weak(expected, value)) {
    }
}

template<typename T>
void atomic_update_minimum(std::atomic<T>& minimum_value, T const& value) noexcept {
    T expected = minimum_value;
    while (expected > value &&
        !minimum_value.compare_exchange_weak(expected, value)) {
    }
}

} // namespace ash
#endif // ASH_UTILITY_STD_ATOMIC_CONATINER_API_H
