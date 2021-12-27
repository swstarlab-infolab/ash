#ifndef ASH_MEMORY_H
#define ASH_MEMORY_H
#include <stdint.h>

namespace ash
{

    typedef struct memory_region
    {
        void *ptr;
        uint64_t size;

        bool is_null() const
        {
            return ptr == nullptr;
        }
    } memrgn_t;

    // forward declrations of memory control modules

    namespace buddy_impl
    {

        struct buddy_block;

    } // namespace buddy_impl

    class buddy_system;
    class portable_buddy_system;

} // namespace ash

#endif // ASH_MEMORY_H
