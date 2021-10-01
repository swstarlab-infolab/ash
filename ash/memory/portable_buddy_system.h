#ifndef ASH_MEMORY_PORTABLE_BUDDY_SYSTEM_H
#define ASH_MEMORY_PORTABLE_BUDDY_SYSTEM_H
#include <ash/memory/buddy_system.h>
#include <unordered_map>

namespace ash {

class portable_buddy_system {
public:
    portable_buddy_system() = default;
    portable_buddy_system(memrgn_t const& rgn, unsigned align, unsigned min_cof);
    void  init(memrgn_t const& rgn, unsigned align, unsigned min_cof);
    void* allocate(uint64_t size);
    void  deallocate(void* p);

    memrgn_t const& rgn() const {
        return buddy.rgn();
    }
    uint64_t max_alloc() const {
        return buddy.max_alloc();
    }

protected:
    std::unordered_map<void*, buddy_impl::buddy_block*> hashmap;
    buddy_system buddy;
};

} // namespace ash
#endif // ASH_MEMORY_PORTABLE_BUDDY_SYSTEM_H
