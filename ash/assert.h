#ifndef ASH_ASSERT_H
#define ASH_ASSERT_H
#include <type_traits>
#include <assert.h>

#define STATIC_ASSERT_IS_TRIVIALLY_COPYABLE(TYPE) \
        static_assert(std::is_trivially_copyable_v<TYPE>,\
        "Since " #TYPE " is a not trivially copyable type!")\

#define STATIC_ASSERT_IS_TRIVIALLY_COPYABLE_WITH_TAG(TYPE, TAG) \
        static_assert(std::is_trivially_copyable_v<TYPE>,\
        "Since " #TYPE " is a not trivially copyable type, "\
        "using" #TAG " for initialization may cause problems!")

#define STATIC_ASSERT_IS_TRIVIALLY_CONSTRUCTABLE(TYPE) \
        static_assert(std::is_trivially_constructible_v<TYPE>,\
        "Since " #TYPE " is a not trivially copyable type!")\

#define STATIC_ASSERT_IS_TRIVIALLY_CONSTRUCTABLE_WITH_TAG(TYPE, TAG) \
        static_assert(std::is_trivially_constructible_v<TYPE>,\
        "Since " #TYPE " is a not trivially constructable type, "\
        "using" #TAG " for initialization may cause problems!")

#define STAITC_ASSERT_MEMSET_SAFETY(TYPE) STATIC_ASSERT_IS_TRIVIALLY_CONSTRUCTABLE_WITH_TAG(TYPE, memset())

#endif // !ASH_ASSERT_H
