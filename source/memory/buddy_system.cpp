#include <ash/memory/buddy_system.h>
#include <ash/pointer.h>
#include <assert.h>
#include <string.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

namespace ash {

buddy_system::buddy_system() {
    memset(&_rgn, 0, sizeof _rgn);
    _align = 0;
    _max_blk_size = 0;
    _flist_v = nullptr;
    _total_allocated_size = 0;
    memset(&_status, 0, sizeof(buddy_impl::buddy_system_status));
}

buddy_system::~buddy_system() {
    _cleanup();
}

buddy_system::buddy_system(memrgn_t const& rgn, unsigned const align, unsigned const min_cof) :
    buddy_system() {
    init(rgn, align, min_cof);
}

void buddy_system::init(memrgn_t const& rgn, unsigned const align, unsigned const min_cof) {
    //TODO: badalloc handling
    using namespace buddy_impl;
    assert(ash::is_aligned_address(_rgn.ptr, align));
    cof_type const root_cof = static_cast<cof_type>(rgn.size / align);
    _rgn = rgn;
    _align = align;
    _max_blk_size = root_cof * align;
    _tbl.init(root_cof, align, min_cof);
    auto block = _block_pool.malloc();
    block->cof = root_cof;
    block->blkidx = 0;
    block->rgn = _rgn;
    block->pair = nullptr;
    block->parent = nullptr;
    block->in_use = false;
    _flist_v = _init_free_list_vec(_tbl.size(), _node_pool);
    _flist_v[0].emplace_front(block);
    _route.reserve(_tbl.max_level());
    _total_allocated_size = 0;
    fprintf(stdout, "Buddy system is online. [%p, %" PRIu64 "]\n", rgn.ptr, rgn.size);
}

void* buddy_system::allocate(uint64_t size) {
    using namespace buddy_impl;
    size += sizeof(buddy_block**);
    buddy_block* block = allocate_block(size);
    if (block == nullptr)
        return nullptr;

    auto const p = static_cast<buddy_block**>(block->rgn.ptr);
    *p = block;
    return p + 1;
}

void buddy_system::deallocate(void* p) {
    if (p == nullptr)
        return;
    deallocate_block(*(static_cast<buddy_block**>(p) - 1));
}

buddy_system::buddy_block* buddy_system::allocate_block(uint64_t const size) {
    using namespace buddy_impl;
    if (ASH_UNLIKELY(size > _max_blk_size))
        return nullptr;

    assert(_route.empty());
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    assert(_route_dbg.empty());
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    blkidx_t bf = _tbl.best_fit(size);
    auto const result = _create_route(bf);
    if (!result.success) {
        _route.clear();
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        _route_dbg.clear();
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        return nullptr; // bad alloc
    }

    assert(!_route.empty());
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    assert(_route.size() == _route_dbg.size());
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    assert(!_flist_v[result.blkidx].empty());
    auto begin = _flist_v[result.blkidx].begin();
    buddy_block* block = *begin;
    _flist_v[result.blkidx].remove_node(begin);

    _route.pop();
    blkidx_t idx_dbg = result.blkidx;
    buddy_block* child[2];
    while (!_route.empty()) {
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        assert(idx_dbg == _route_dbg.peek());
        _route_dbg.pop();
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        child[0] = _block_pool.malloc();
        child[1] = _block_pool.malloc();
        _split_block(block, child[0], child[1], _tbl);
        block->in_use = true;
        buddy_block*& target = child[_route.peek()];
        buddy_block*& spare = child[!_route.peek()];
        assert(_flist_v[target->blkidx].empty());
        _ash_unused(idx_dbg);
        idx_dbg = target->blkidx;
        spare->inv = _flist_v[spare->blkidx].emplace_front(spare).node();

        // update states
        block = target;
        _route.pop();
    }
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    _route_dbg.pop();
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING

    assert(block->in_use == false);

    block->in_use = true;
    block->inv = nullptr;
    _status.total_allocated += 1;

    assert(_route.empty());
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    assert(_route_dbg.empty());
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING

    assert(size <= (uint64_t)(block->cof * _align));
    _total_allocated_size += block->cof * _align;
    return block;
}

void buddy_system::deallocate_block(buddy_block* blk) {
    _total_allocated_size -= blk->cof * _align;
    _deallocate(blk);
}
void buddy_system::_cleanup_free_list_vec(unsigned const size, buddy_impl::free_list_t* v) {
    using namespace buddy_impl;
    for (unsigned i = 0; i < size; ++i) {
        assert(i == 0 || v[i].empty());
        v[i].~free_list_t();
    }
    delete v;
}

void buddy_system::_split_block(buddy_block* parent, buddy_block* left, buddy_block* right, buddy_impl::buddy_table const& tbl) {
    using namespace buddy_impl;
    assert(parent->in_use == false);

    auto const left_block_index = [&tbl](buddy_block const* parent) -> blkidx_t {
        auto const prop = tbl.property(parent->blkidx);
        blkidx_t const parent_lv_base = parent->blkidx - prop.offset;
        if (prop.check(UniqueBuddyBlock))
            return parent_lv_base + 1;
        if (parent->cof & 0x1u)
            return parent_lv_base + 2;
        return parent_lv_base + 2 + (prop.offset != 0);
    };

    auto const right_block_index = [&tbl](buddy_block const* parent) -> blkidx_t {
        auto const prop = tbl.property(parent->blkidx);
        blkidx_t const parent_lv_base = parent->blkidx - prop.offset;
        if (prop.check(UniqueBuddyBlock))
            return parent_lv_base + 1 + (parent->cof & 0x1u);
        if (parent->cof & 0x1u)
            return parent_lv_base + 3;
        return parent_lv_base + 2 + (prop.offset != 0);
    };

    // left
    left->cof = parent->cof / 2 + parent->cof % 2;
    left->rgn.ptr = parent->rgn.ptr;
    left->rgn.size = tbl.align() * left->cof;
    left->pair = right;
    left->parent = parent;
    left->in_use = false;
    left->inv = nullptr;
    left->blkidx = left_block_index(parent);

    // right
    right->cof = parent->cof - left->cof;
    right->rgn.ptr = seek_pointer(parent->rgn.ptr, left->rgn.size);
    right->rgn.size = parent->rgn.size - left->rgn.size;
    right->pair = left;
    right->parent = parent;
    right->in_use = false;
    right->inv = nullptr;
    right->blkidx = right_block_index(parent);
}

/*
 * * Root: 200
 * Minimum coefficient: 3
 *
 * Coefficient tree
 *              +---          [232]             ---> 232 [Root]
 *              |               |
 *              |             [116]             ---> 116
 *   Linear  <--+               |
 *              |             [58]              ---> 58
 *              |               |
 *              +---          [29]              ---> 29
 *              |            /    \
 *              |         [15]     [14]         ---> 15, 14
 *   Binary  <--+        /  \       /  \
 *              |      [8]  [7]    [7]  [7]     ---> 8, 7 [A1B3-Pattern]
 *              |      / \  / \    / \   / \
 *              +--- [4][4][4][3] [4][3][4][3]  ---> 4, 3 [A3B1-Pattern]
 *
 * Buddy table
 * - Flag: Unique (U), Frequent (F), Rare (R), A1B3-Pattern, A3B1-Pattern
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * | Idx | Lev |  Cof  | U | F | R | Pattern | Dist | Off |
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * |  0  |  0  |  232  | # |   |   |   N/A   |   0  |  0  | => 0: U, Root
 * |  1  |  1  |  116  | # |   |   |   N/A   |   1  |  0  | => 1: U
 * |  2  |  2  |   58  | # |   |   |   N/A   |   1  |  0  | => 2: U
 * |  3  |  3  |   29  | # |   |   |   N/A   |   1  |  0  | => 3: U
 * |  4  |  4  |   15  |   |   | # |   A3B1  |   1  |  0  | => 4: R-A3B1*
 * |  5  |  4  |   14  |   |   | # |   A3B1  |   2  |  1  | => 5: R-A3B1*
 * |  6  |  5  |    8  |   |   | # |   A1B3  |   2  |  0  | => 6: R-A1B3
 * |  7  |  5  |    7  |   | # |   |   A1B3  |   3  |  1  | => 7: F-A1B3
 * |  8  |  6  |    4  |   | # |   |   A3B1  |   2  |  0  | => 8: F-A3B1
 * |  9  |  6  |    3  |   |   | # |   A3B1  |   3  |  1  | => 9: R-A3B1
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * Note that the patterns of a first binary level (4) are assumed to be R-A3B1*
 *
 * Initial state of:
 * Free-list vector         | Allocation tree (* means free node)
 * +-----+-----------+      |               [232:0x00]*
 * | Idx | Free-list |      |
 * +-----+-----------+      |
 * |  0  |    0x00   |      |
 * |  1  |    NULL   |      |
 * |  2  |    NULL   |      |
 * |  3  |    NULL   |      |
 * |  4  |    NULL   |      |
 * |  5  |    NULL   |      |
 * |  6  |    NULL   |      |
 * |  7  |    NULL   |      |
 * |  8  |    NULL   |      |
 * |  9  |    NULL   |      |
 * +-----+-----------+      |
 *
 * Create a route of seed 9 for a first allocation:
 * +------+-----+--------+------------+------+--------+---------------------+
 * | Step | Idx | Lookup | Properties | Cand | Parent |        Route        |
 * +------+-----+--------+------------+------+--------+---------------------+
 * |   0  |  9  |  MISS  |   R-A3B1   | 8, 9 |  6, 7  | 8                   |
 * |  1-1 |  6  |  MISS  |   R-A1B3   |   6  |    4   |                     |
 * |  1-2 |  7  |  MISS  |   F-A1B3   | 6, 7 |  4, 5  | 8->7                |
 * |  2-1 |  5  |  MISS  |   R-A3B1*  |   5  |    3   |                     |
 * |  2-2 |  4  |  MISS  |   R-A3B1*  |   4  |    3   | 8->7->4             |
 * |   3  |  3  |  MISS  |      U     |   3  |    2   | 8->7->4->3          |
 * |   4  |  2  |  MISS  |      U     |   2  |    1   | 8->7->4->3->2       |
 * |   5  |  1  |  MISS  |      U     |   1  |    0   | 8->7->4->3->2->1    |
 * |   6  |  0  |   HIT  |      U     |   0  |   NULL | 8->7->4->3->2->1->0 |
 * +------+-----+--------+------------+------+--------+---------------------+
 *
 * States after the first allocation:
 * Free-list vector         | Allocation tree (* means free node)
 * +-----+-----------+      |                          [232:0x00]
 * | Idx | Free-list |      |                          /         \
 * +-----+-----------+      |                  [116:0x10]        [116:0x11]*
 * |  0  |    NULL   |      |                  /         \
 * |  1  |    0x11   |      |          [58:0x20]         [58:0x21]*
 * |  2  |    0x21   |      |           |       \
 * |  3  |    0x31   |      |        [29:0x30]  [29:0x31]*
 * |  4  |    0x41   |      |           |     \
 * |  5  |    NULL   |      |      [15:0x40]  [14:0x41]*
 * |  6  |    NULL   |      |        |      \
 * |  7  |    0x50   |      |    [8:0x50]*  [7:0x51]
 * |  8  |    NULL   |      |                /     \
 * |  9  |    0x61   |      |           [4:0x60]  [3:0x61]*
 * +-----+-----------+      |              ^
 *                          |              |
 *                          |              +-- return this (request: 3, result 4)
 *
 * Create a route of seed 9 for a second allocation:
 * +------+-----+--------+------------+------+--------+---------------------+
 * | Step | Idx | Lookup | Properties | Cand | Parent |        Route        |
 * +------+-----+--------+------------+------+--------+---------------------+
 * |   0  |  9  |   HIT  |   R-A3B1   | 8, 9 |  6, 7  | 9 (Cache hit)       |
 * +------+-----+--------+------------+------+--------+---------------------+
 *
 * States after the second allocation:
 * Free-list vector         | Allocation tree (* means free node)
 * +-----+-----------+      |                          [232:0x00]
 * | Idx | Free-list |      |                          /         \
 * +-----+-----------+      |                  [116:0x10]        [116:0x11]*
 * |  0  |    NULL   |      |                  /         \
 * |  1  |    0x11   |      |          [58:0x20]         [58:0x21]*
 * |  2  |    0x21   |      |           |       \
 * |  3  |    0x31   |      |        [29:0x30]  [29:0x31]*
 * |  4  |    0x41   |      |           |     \
 * |  5  |    NULL   |      |      [15:0x40]  [14:0x41]*
 * |  6  |    NULL   |      |        |      \
 * |  7  |    0x50   |      |    [8:0x50]*  [7:0x51]
 * |  8  |    NULL   |      |                /     \
 * |  9  |    NULL   |      |           [4:0x60]  [3:0x61]*
 * +-----+-----------+      |                        ^
 *                          |                        |
 *                          |                        +-- here (request: 3, result 3)
 *
 */

} // !namespace ash
