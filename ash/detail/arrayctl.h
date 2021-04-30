#ifndef ASH_DETAIL_ARRAYCTL_H
#define ASH_DETAIL_ARRAYCTL_H
#include <ash/config.h>
#include <type_traits>

namespace ash {
namespace detail {

namespace _arrayctl {

template <typename T>
using pod_type_tracing = std::integral_constant<bool, std::is_pod<T>::value>;

template <typename T>
using pod_type = std::integral_constant<bool, true>;

template <typename T>
using non_pod_t = std::integral_constant<bool, false>;

template <typename T>
void copy_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size, pod_type<T>) {
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T>
void copy_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size, non_pod_t<T>) {
    for (size_t i = 0; i < size; ++i)
        dst[i] = src[i];
}

template <typename T>
void move_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size, pod_type<T>) {
    memcpy(dst, src, sizeof(T) * size);
}

template <typename T>
void move_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size, non_pod_t<T>) {
    for (size_t i = 0; i < size; ++i)
        dst[i] = std::move(src[i]);
}

template <typename T>
void clear_array(T* dst, size_t size, pod_type<T>) {
    memset(dst, 0, sizeof(T) * size);
}

template <typename T>
void clear_array(T* dst, size_t size, non_pod_t<T>) {
    for (size_t i = 0; i < size; ++i)
        dst.~T();
}

} // namespace _arrayctl

template <typename T>
void copy_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size) {
    using namespace _arrayctl;
    copy_array(dst, src, size, pod_type_tracing<T>());
}

template <typename T>
void move_array(T* ASH_RESTRICT dst, T* ASH_RESTRICT src, size_t size) {
    using namespace _arrayctl;
    move_array(dst, src, size, pod_type_tracing<T>());
}

template <typename T>
void clear_array(T* dst, size_t size) {
    using namespace _arrayctl;
    clear_array(dst, size, pod_type_tracing<T>());
}

} // !namespace detail

} // !namespace ash
#endif // ASH_DETAIL_ARRAYCTL_H
