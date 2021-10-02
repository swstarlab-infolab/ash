#include <ash/memory/portable_buddy_system.h>

namespace ash {

portable_buddy_system::portable_buddy_system(memrgn_t const& rgn, unsigned align, unsigned min_cof) {
    init(rgn, align, min_cof);
}

void portable_buddy_system::init(memrgn_t const& rgn, unsigned align, unsigned min_cof) {
    buddy.init(rgn, align, min_cof);
}

void* portable_buddy_system::allocate(uint64_t size) {
    using namespace buddy_impl;
    buddy_block* block = buddy.allocate_block(size);
    if (block == nullptr)
        return nullptr;
    void* p = block->rgn.ptr;
    assert(hashmap.find(p) == hashmap.end());
    hashmap.emplace(p, block);
    return p;
}

void portable_buddy_system::deallocate(void* p) {
    using namespace buddy_impl;
    assert(hashmap.find(p) != hashmap.end());
    buddy_block* block = hashmap.at(p);
    buddy.deallocate_block(block);
    hashmap.erase(p);
}

}