#ifndef ASH_MPL_PARAMETER_PACK_TRAITS_H
#define ASH_MPL_PARAMETER_PACK_TRAITS_H
#include <type_traits>

namespace ash {
namespace mpl {

template <typename ...Rest>
struct is_typeseq_pointer;

template <typename T>
struct is_typeseq_pointer<T> {
    static constexpr bool value = std::is_pointer_v<T>;
};

template <typename T, typename ...Rest>
struct is_typeseq_pointer<T, Rest...> {
    static constexpr bool value = std::is_pointer_v<T> && is_typeseq_pointer<Rest...>::value;
};

template <typename ...Rest>
struct is_typeseq_array;

template <typename T>
struct is_typeseq_array<T> {
    static constexpr bool value = std::is_array_v<T>;
};

template <typename T, typename ...Rest>
struct is_typeseq_array<T, Rest...> {
    static constexpr bool value = std::is_array_v<T> && is_typeseq_array<Rest...>::value;
};

} // namespace mpl
} // namespace ash
#endif // ASH_MPL_PARAMETER_PACK_TRAITS_H
