#ifndef ASH_PP_TENSOR_H
#define ASH_PP_TENSOR_H
#include <ash/pp/pparg.h>

#define _ASH_TENSOR_1(X) [X]
#define _ASH_TENSOR_2(X,Y) [Y][X]
#define _ASH_TENSOR_3(X,Y,Z) [Z][Y][X]
#define _ASH_TENSOR_4(X,Y,Z,W) [W][Z][Y][X]
#define __ASH_TENSOR_N(N) _ASH_TENSOR_##N
#define _ASH_TENSOR_N(N) __ASH_TENSOR_N(N)
#define ASH_TENSOR(Name, ElemTy, ...) \
    ElemTy Name ASH_PP_EXPAND(_ASH_TENSOR_N(ASH_PP_NARG(__VA_ARGS__)) (__VA_ARGS__))
#define ASH_TENSOR_PTR(Name, ElemTy, ...) \
    ElemTy (*Name) ASH_PP_EXPAND(_ASH_TENSOR_N(ASH_PP_NARG(__VA_ARGS__)) (__VA_ARGS__))
#define ASH_TENSOR_REF(Name, ElemTy, ...) \
    ElemTy (&Name) ASH_PP_EXPAND(_ASH_TENSOR_N(ASH_PP_NARG(__VA_ARGS__)) (__VA_ARGS__))
#define _ASH_PP_MUL_1(X) (X)
#define _ASH_PP_MUL_2(X, Y) ((X) * (Y))
#define _ASH_PP_MUL_3(X, Y, Z) ((X) * (Y) * (Z))
#define _ASH_PP_MUL_4(X, Y, Z, W) ((X) * (Y) * (Z) * (W))
#define __ASH_PP_MUL(N) _ASH_PP_MUL_##N
#define _ASH_PP_MUL(N) __ASH_PP_MUL(N)
#define ASH_PP_MUL(...) ASH_PP_EXPAND(_ASH_PP_MUL(ASH_PP_NARG(__VA_ARGS__)) (__VA_ARGS__))
#define ASH_TENSOR_SIZE(ElemTy, ...) (sizeof(ElemTy) * ASH_PP_MUL(__VA_ARGS__))
#define ASH_TENSOR_ELEM_COUNT(...) (ASH_PP_MUL(__VA_ARGS__))
#define ASH_ALLOC_TENSOR(ElemTy, ...) \
    (ElemTy(*)\
        ASH_PP_EXPAND(_ASH_TENSOR_N(ASH_PP_NARG(__VA_ARGS__)) (__VA_ARGS__))) malloc(ASH_TENSOR_SIZE(ElemTy, __VA_ARGS__))

#endif // ASH_PP_TENSOR_H
