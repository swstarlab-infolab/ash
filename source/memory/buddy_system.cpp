#include <ash/memory/buddy_system.h>
#include <ash/pointer.h>
#include <ash/utility/dbg_log.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
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
    auto block = _block_pool.allocate();
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
    ASH_DMESG("Buddy system is online. [%p, %" PRIu64 "]", rgn.ptr, rgn.size);
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
    assert(block != nullptr);
    _flist_v[result.blkidx].remove_node(begin);

    _route.pop();
    blkidx_t idx_dbg = result.blkidx;
    buddy_block* child[2];
    while (!_route.empty()) {
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        assert(idx_dbg == _route_dbg.peek());
        _route_dbg.pop();
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        child[0] = _block_pool.allocate();
        child[1] = _block_pool.allocate();
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

    assert(size <= static_cast<uint64_t>(block->cof * _align));
    _total_allocated_size += block->cof * _align;
    return block;
}

void buddy_system::deallocate_block(buddy_block* blk) {
    _total_allocated_size -= blk->cof * _align;
    _deallocate(blk);
}

buddy_impl::free_list_t* buddy_system::_init_free_list_vec(unsigned const size, buddy_impl::free_list_t::pool_type& pool) {
    using namespace buddy_impl;
    free_list_t* v = static_cast<free_list_t*>(calloc(size, sizeof(free_list_t)));
    if (v != nullptr) {
        for (unsigned i = 0; i < size; ++i)
            new (&v[i]) free_list_t(pool);
    }
    else {
        fprintf(stderr, "Bad alloc occured during initialize a free list vector of the buddy system!\n");
    }
    return v;
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

    auto const left_block_index = [&tbl](buddy_block const* parent_) -> blkidx_t {
        auto const prop = tbl.property(parent_->blkidx);
        blkidx_t const parent_lv_base = parent_->blkidx - prop.offset;
        if (prop.check(UniqueBuddyBlock))
            return parent_lv_base + 1;
        if (parent_->cof & 0x1u)
            return parent_lv_base + 2;
        return parent_lv_base + 2 + (prop.offset != 0);
    };

    auto const right_block_index = [&tbl](buddy_block const* parent_) -> blkidx_t {
        auto const prop = tbl.property(parent_->blkidx);
        blkidx_t const parent_lv_base = parent_->blkidx - prop.offset;
        if (prop.check(UniqueBuddyBlock))
            return parent_lv_base + 1 + (parent_->cof & 0x1u);
        if (parent_->cof & 0x1u)
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

void buddy_system::_deallocate(buddy_block* block) {
    assert(block->in_use == true);
    block->in_use = false;
    buddy_block* pair = block->pair;
    if (block->pair == nullptr || pair->in_use) {
        block->inv = _flist_v[block->blkidx].emplace_back(block).node();
        return;
    }
    buddy_block* parent = block->parent;
    _flist_v[pair->blkidx].remove_node(pair->inv);
    _block_pool.deallocate(block);
    _block_pool.deallocate(pair);
    _deallocate(parent);
    _status.total_deallocated += 1;
}

void buddy_system::_cleanup() {
    //TODO: Implementation
    if (_flist_v == nullptr)
        return; // system is not initialized yet.
    if (_flist_v[0].empty()) {
        fprintf(stderr, "Buddy system detects memory leak!\n");
    }
    _cleanup_free_list_vec(_tbl.size(), _flist_v);
    _tbl.clear();
}

buddy_system::buddy_block* buddy_system::_acquire_block(buddy_impl::blkidx_t const bidx) const {
    using namespace buddy_impl;
    free_list_t& list = _flist_v[bidx];
    if (list.empty())
        return nullptr;
    auto begin = list.begin();
    buddy_block* block = *begin;
    _flist_v[bidx].remove_node(begin);
    return block;
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

buddy_system::routing_result buddy_system::_create_route(buddy_impl::blkidx_t bidx) {
    using namespace buddy_impl;
    auto const append_idx_to_route_if_cached = [this](blkidx_t const idx) -> bool {
        auto const prop = _tbl.property(idx);
        if (!_flist_v[idx].empty()) {
            _route.push(prop.offset);
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
            _route_dbg.push(idx);
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
            return true;
        }
        return false;
    };

#ifdef ASH_BUDDY_SYSTEM_PREVENT_ROOT_ALLOC
    // Terminate if the target block is a root node
    if (ASH_UNLIKELY(blkidx == 0))
        return routing_result{ false, 0 }; // bad alloc
#endif

    // Lookup caches of requested block index
    if (append_idx_to_route_if_cached(bidx))
        return routing_result{ true, bidx };

    // If the requested block is R-A3B1,
    // restart the routine to allocate a neighbor block
    auto prop = _tbl.property(bidx);
    if (prop.check(RareBuddyBlock | A3B1Pattern))
        return _create_route(bidx - 1);

    // Append a current index to the route
    _route.push(prop.offset);
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    _route_dbg.push(bidx);
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING

    // If the requested block is R-A1B3,
    // restart the routine to allocate its parent block
    if (prop.check(RareBuddyBlock | A1B3Pattern))
        return _create_route(bidx - prop.dist);

    do {
        // Move the index to the beginning of the previous level
        bidx -= prop.dist;
        prop = _tbl.property(bidx);
        if (prop.check(UniqueBuddyBlock)) {
            if (append_idx_to_route_if_cached(bidx))
                return routing_result{ true, bidx };
            // Append the current index to the route (Unique: 0)
            _route.push(0);
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
            _route_dbg.push(bidx);
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        }
        else if (prop.check(A1B3Pattern)) {
            if (append_idx_to_route_if_cached(bidx))
                return routing_result{ true, bidx };
            if (append_idx_to_route_if_cached(bidx + 1))
                return routing_result{ true, bidx + 1 };
            // Append the frequent block index to the route (A1B3 => Offset of B: 1)
            _route.push(1);
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
            _route_dbg.push(bidx + 1);
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        }
        else if (prop.check(A3B1Pattern)) {
            if (append_idx_to_route_if_cached(bidx + 1))
                return routing_result{ true, bidx + 1 };
            if (append_idx_to_route_if_cached(bidx))
                return routing_result{ true, bidx };
            // Append the frequent block index to the route (A3B1 => Offset of A: 0)
            _route.push(0);
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
            _route_dbg.push(bidx);
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
        }
    } while (bidx > 0);

    if (append_idx_to_route_if_cached(0))
        return routing_result{ true, 0 };

    return routing_result{ false, bidx };
}

} // !namespace ash
