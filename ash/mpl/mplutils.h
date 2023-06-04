#ifndef ASH_MPL_SEQUENCE_GENERATOR_H
#define ASH_MPL_SEQUENCE_GENERATOR_H
#include <type_traits>

namespace ash {
namespace mpl {

template<int ...>
struct sequence_type {

};

template<int N, int ...S>
struct sequence_generator : sequence_generator < N - 1, N - 1, S... > {
};

template<int ...S>
struct sequence_generator < 0, S... > {
    typedef sequence_type<S...> type;
};

template <typename T>
class has_CONSTANT {
    template <typename C> static std::true_type test(decltype(&C::CONSTANT));
    template <typename> static std::false_type test(...);

public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

} // !namespace mpl
} // !namespace ash

#endif // ASH_MPL_SEQUENCE_GENERATOR_H
