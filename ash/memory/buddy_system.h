#ifndef _ASH_MEMORY_BUDDY_SYSTEM_H_
#define _ASH_MEMORY_BUDDY_SYSTEM_H_
#include <ash/memory.h>
#include <ash/memory/buddy_table.h>
#include <ash/pooling_list.h>
#include <ash/bitstack.h>
#include <string.h> // memset

namespace ash {

namespace buddy_impl {

struct buddy_block;

using free_list_t = pooling_list<buddy_block*>;

struct buddy_system_status {
    uint64_t total_allocated;
    uint64_t total_deallocated;
};

struct buddy_block {
    cof_type cof;
    buddy_block* pair;
    buddy_block* parent;
    bool in_use;
    free_list_t::node_pointer inv;
    blkidx_t blkidx;
    memrgn_t rgn;
};

} // namespace buddy_impl

class buddy_system {
    using buddy_block = buddy_impl::buddy_block;
public:
    buddy_system();
    ~buddy_system();
    buddy_system(memrgn_t const& rgn, unsigned align, unsigned min_cof);
    void init(memrgn_t const& rgn, unsigned align, unsigned min_cof);
    void* allocate(uint64_t size);
    void deallocate(void* p);
    buddy_block* allocate_block(uint64_t size);
    void deallocate_block(buddy_block* blk);

    memrgn_t const& rgn() const {
        return _rgn;
    }

    uint64_t max_alloc() const {
        return _max_blk_size;
    }

protected:
    struct routing_result {
        bool success;
        buddy_impl::blkidx_t blkidx;
    };

    static buddy_impl::free_list_t* _init_free_list_vec(unsigned size, buddy_impl::free_list_t::pool_type& pool);
    static void _cleanup_free_list_vec(unsigned size, buddy_impl::free_list_t* v);
    static void _split_block(buddy_block* parent, buddy_block* left, buddy_block* right, buddy_impl::buddy_table const& tbl);

    void _deallocate(buddy_block* block);
    void _cleanup();
    buddy_block* _acquire_block(buddy_impl::blkidx_t bidx) const;
    routing_result _create_route(buddy_impl::blkidx_t bidx);

    memrgn_t _rgn;
    unsigned _align;
    uint64_t _max_blk_size;
    buddy_impl::free_list_t::pool_type _node_pool;
    ash::unordered_object_pool<buddy_block> _block_pool;
    buddy_impl::free_list_t* _flist_v;
    bitstack _route;
    buddy_impl::buddy_system_status _status;
    buddy_impl::buddy_table _tbl;
#ifdef ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    stack<unsigned> _route_dbg;
#endif // !ASH_DEBUG_ENABLE_BUDDY_ROUTE_CORRECTNESS_CHECKING
    int64_t _total_allocated_size;
};

template<typename T = void>
class buddy_allocator {
public:
    using value_type = T;
    using size_type = std::size_t;
    using pointer = T*;
    using const_pointer = const T*;
    using difference_type = typename std::pointer_traits<pointer>::difference_type;

    explicit buddy_allocator(buddy_system& buddy) noexcept :
        _buddy(buddy) {
    }

    ~buddy_allocator() noexcept {
    }

    template<typename U>
    buddy_allocator(buddy_allocator<U> const& other) noexcept :
        _buddy(other.buddy) {
    }

    T* allocate(size_t size, const void* = 0) {
        return _buddy.allocate(size);
    }

    void deallocate(T* ptr, size_t /*size*/) {
        _buddy.deallocate(ptr);
    }

    template<typename U>
    struct rebind {
        typedef buddy_allocator<U> other;
    };

private:
    buddy_system& _buddy;
};

} // !namespace ash

#endif // _ASH_MEMORY_BUDDY_SYSTEM_H_
