#ifndef ASH_ALGORITHM_H
#define ASH_ALGORITHM_H
#include <ash/config.h>
#include <stdint.h>
#include <ash/pp/parameter_pack_traits.h>

namespace ash {

namespace quicksort_impl {

template <typename T>
ASH_FORCEINLINE void expand_swap_arrays(int64_t off1, int64_t off2, T&& arr) {
    using std::swap;
    swap(arr[off1], arr[off2]);
}

template <typename T, typename ...Rest>
ASH_FORCEINLINE void expand_swap_arrays(int64_t off1, int64_t off2, T&& arr, Rest&&... rest) {
    using std::swap;
    swap(arr[off1], arr[off2]);
    expand_swap_arrays(off1, off2, std::forward<Rest>(rest)...);
}

template <typename ValueType, typename KeyArr, typename ...Arrs>
int64_t _partition(KeyArr&& base, int64_t const low, int64_t const high, Arrs&&... cor) {
    using value_type = ValueType;
    value_type pivot = base[high];
    int64_t i = low - 1;
    using std::swap;

    for (int64_t j = low; j < high; ++j) {
        if (base[j] < pivot) {
            i += 1;
            swap(base[i], base[j]);
            expand_swap_arrays(i, j, std::forward<Arrs>(cor)...);
        }
    }
    swap(base[i + 1l], base[high]);
    expand_swap_arrays(i + 1, high, std::forward<Arrs>(cor)...);
    return i + 1;
}

template <typename ValueType, typename Array>
int64_t _partition(Array&& arr, int64_t const low, int64_t const high) {
    using value_type = ValueType;
    value_type pivot = arr[high];
    int64_t i = low - 1;
    using std::swap;

    for (int64_t j = low; j < high; ++j) {
        if (arr[j] < pivot) {
            i += 1;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1l], arr[high]);
    return i + 1;
}

template <typename T, typename _T = std::remove_reference_t<T>>
ASH_FORCEINLINE
std::enable_if_t<std::is_pointer_v<_T> || std::is_array_v<_T>, int64_t>
partition(T&& arr, int64_t const low, int64_t const high) {
    using value_type = std::conditional_t<std::is_pointer_v<_T>, std::remove_pointer_t<_T>, std::remove_extent_t<_T>>;
    return _partition<value_type>(std::forward<T>(arr), low, high);
}

template <typename SeqContainer, typename _DeRef = std::remove_reference_t<SeqContainer>>
ASH_FORCEINLINE
std::enable_if_t<!std::is_pointer_v<_DeRef> && !std::is_array_v<_DeRef>, int64_t>
partition(SeqContainer&& vec, int64_t const low, int64_t const high) {
    using value_type = typename _DeRef::value_type;
    return _partition<value_type>(std::forward<SeqContainer>(vec), low, high);
}

template <
    typename KeyArr,
    typename _DeRef = std::remove_reference_t<KeyArr>,
    typename ...Arrs>
ASH_FORCEINLINE
std::enable_if_t<std::is_pointer_v<_DeRef> || std::is_array_v<_DeRef>, int64_t>
partition(KeyArr&& base, int64_t const low, int64_t const high, Arrs&&... cor) {
    using value_type = std::conditional_t<std::is_pointer_v<_DeRef>, std::remove_pointer_t<_DeRef>, std::remove_extent_t<_DeRef>>;
    return _partition<value_type>(std::forward<KeyArr>(base), low, high, std::forward<Arrs>(cor)...);
}

template <
    typename KeyArr,
    typename _DeRef = std::remove_reference_t<KeyArr>,
    typename ...Arrs>
ASH_FORCEINLINE
std::enable_if_t<!std::is_pointer_v<_DeRef> && !std::is_array_v<_DeRef>, int64_t>
partition(KeyArr&& base, int64_t const low, int64_t const high, Arrs&&... cor) {
    using value_type = typename _DeRef::value_type;
    return _partition<value_type>(std::forward<KeyArr>(base), low, high, std::forward<Arrs>(cor)...);
}

} // namespace quicksort_impl 

template <typename KeyArr>
void quicksort(KeyArr&& base, int64_t low, int64_t high) {
    if (low < high) {
        int64_t const pi = quicksort_impl::partition(std::forward<KeyArr>(base), low, high);
        quicksort(std::forward<KeyArr>(base), low, pi - 1);
        quicksort(std::forward<KeyArr>(base), pi + 1, high);
    }
}

template <typename KeyArr, typename ...Arrs>
void quicksort(KeyArr&& base, int64_t low, int64_t high, Arrs&&... cor) {
    if (low < high) {
        int64_t const pi = quicksort_impl::partition(std::forward<KeyArr>(base), low, high, std::forward<Arrs>(cor)...);
        quicksort(std::forward<KeyArr>(base), low, pi - 1, std::forward<Arrs>(cor)...);
        quicksort(std::forward<KeyArr>(base), pi + 1, high, std::forward<Arrs>(cor)...);
    }
}

} // !namespace ash
#endif // ASH_ALGORITHM_H
