#ifndef ASH_VERTORIZATION_H
#define ASH_VERTORIZATION_H

namespace ash {

template <typename ValueTy>
struct dim2 {
    using value_type = ValueTy;
    value_type x;
    value_type y;
    value_type size() const {
        return x * y;
    }
};

template <typename ValueTy>
struct dim3 {
    using value_type = ValueTy;
    value_type x;
    value_type y;
    value_type z;
    value_type size() const {
        return x * y * z;
    }
};

} // !namespace ash

#endif // ASH_VERTORIZATION_H
