#ifndef ASH_MEMORY_RAII_BUFFER_H
#define ASH_MEMORY_RAII_BUFFER_H
#include <ash/detail/malloc.h>
#include <memory>

namespace ash {

namespace detail {

struct c_malloc_deleter {
    void operator()(void* m) const { free(m); }
};

struct aligned_deleter {
    void operator()(void* m) const { aligned_free(m); }
};

template <typename T>
struct objarr_deleter {
    void operator()(T* m) const { delete[] m; }
};

} // namespace detail

template <typename T>
using raii_buffer = std::unique_ptr<T, detail::c_malloc_deleter>;

template <typename T>
using aligned_raii_buffer = std::unique_ptr<T, detail::aligned_deleter>;

template <typename T>
using objarr_raii_buffer = std::unique_ptr<T, detail::objarr_deleter<T> >;

template <typename T>
raii_buffer<T> make_raii_buffer(size_t const size) noexcept {
    return raii_buffer<T>{ static_cast<T*>(malloc(size)) };
}

template <typename T>
raii_buffer<T> make_aligned_raii_buffer(size_t const size, unsigned const align) noexcept {
    return raii_buffer<T>{ static_cast<T*>(aligned_malloc(size, align)) };
}

template <typename T>
objarr_raii_buffer<T> make_objarr_raii_buffer(size_t const count) noexcept {
    return objarr_raii_buffer<T>{ new(std::nothrow) T[count] };
}

} // namespace ash

#endif // ASH_MEMORY_RAII_BUFFER_H
