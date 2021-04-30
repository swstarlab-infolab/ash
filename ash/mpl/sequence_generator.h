#ifndef ASH_MPL_SEQUENCE_GENERATOR_H
#define ASH_MPL_SEQUENCE_GENERATOR_H

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

} // !namespace mpl
} // !namespace ash

#endif // ASH_MPL_SEQUENCE_GENERATOR_H
