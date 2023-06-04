#ifndef ASH_MEMORY_UNORDERED_OBJECT_POOL_H
#define ASH_MEMORY_UNORDERED_OBJECT_POOL_H
#include <ash/memory/segregated_storage.h>
#include <ash/numeric.h>
#include <stack>
#include <assert.h>

namespace ash {

template <typename ValueType, size_t ClusterSize = 1024, typename Allocator = std::allocator<ValueType>>
class unordered_object_pool : noncopyable {
public:
    using value_type = ValueType;
    using alloc_type = Allocator;
    constexpr static size_t cluster_size = ClusterSize;
    constexpr static double recycle_factor = 0.5F;

    explicit unordered_object_pool(size_t reserved = cluster_size, const alloc_type& alloc = alloc_type{});
    ~unordered_object_pool() noexcept;
    template <typename ...Args> value_type* construct(Args&& ...args);
    value_type* allocate();
    value_type* allocate_zero_initialized();
    void destroy(value_type* p);
    void deallocate(value_type* p);
    void reserve(size_t required);

    ASH_FORCEINLINE size_t num_clusters() const {
        return _num_nodes;
    }

    ASH_FORCEINLINE size_t capacity() const {
        return _capacity;
    }

protected:
    struct block_type {
        void* key;
        value_type value;
    };
    constexpr static size_t cluster_region_size = sizeof(block_type) * cluster_size;
    using cluster_t = segregated_storage;

    struct cluster_node {
        cluster_node() = delete;
        char buffer[cluster_region_size];
        cluster_t cluster;
        cluster_node* next;
        cluster_node* prev;
        bool stacked;
    };

    using node_alloc_t = typename std::allocator_traits<alloc_type>::
        template rebind_alloc<cluster_node>;
    using node_stack_t = ::std::stack<cluster_node*>;

    ASH_FORCEINLINE cluster_node* _allocate_node() {
        cluster_node* node = std::allocator_traits<node_alloc_t>::allocate(_node_alloc, 1);
        new(&node->cluster) cluster_t(node->buffer, cluster_region_size, sizeof(block_type));
        node->next = nullptr;
        node->prev = nullptr;
        node->stacked = false;
        _num_nodes += 1;
        _capacity += cluster_size;
        return node;
    }

    ASH_FORCEINLINE void _deallocate_node(cluster_node* node) {
        node->cluster.~cluster_t();
        std::allocator_traits<node_alloc_t>::deallocate(_node_alloc, node, 1);
        _num_nodes -= 1;
        _capacity -= cluster_size;
    }

    ASH_FORCEINLINE void _insert_back_to(cluster_node* target, cluster_node* node) {
        cluster_node* next = target->next;
        node->next = target->next;
        node->prev = target;
        target->next = node;
        if (next != nullptr)
            next->prev = node;
    }

    ASH_FORCEINLINE cluster_node* _detach_node(cluster_node* node) noexcept {
        cluster_node* prev = node->prev;
        cluster_node* next = node->next;
        if (prev != nullptr)
            prev->next = next;
        if (next != nullptr)
            next->prev = prev;
        return next;
    }

    alloc_type _alloc;
    size_t _capacity;
    size_t _num_nodes;
    node_alloc_t _node_alloc;
    node_stack_t _node_stack;
    cluster_node* _curr;
};

template <typename ValueType, size_t ClusterSize, typename Allocator>
unordered_object_pool<ValueType, ClusterSize, Allocator>::unordered_object_pool(size_t reserved, const alloc_type& alloc) :
    _alloc(alloc),
    _capacity(0),
    _num_nodes(0),
    _node_alloc(alloc) {
    assert(_capacity % cluster_size == 0);
    _curr = _allocate_node();
    reserve(reserved);
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
unordered_object_pool<ValueType, ClusterSize, Allocator>::~unordered_object_pool() noexcept {
#ifdef MIXX_DEBUG_ENABLE_OBJECT_LEAK_DETECTION
    assert(_curr->prev == nullptr);
    assert(_curr->next == nullptr);
    assert(_node_stack.size() == _num_nodes - 1);
#endif // !MIXX_DEBUG_ENABLE_OBJECT_LEAK_DETECTION
    _deallocate_node(_curr);
    while (!_node_stack.empty()) {
        cluster_node* p;
        p = _node_stack.top();
        _node_stack.pop();
        _deallocate_node(p);
    }
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
template <typename ... Args>
typename unordered_object_pool<ValueType, ClusterSize, Allocator>::value_type* unordered_object_pool<ValueType, ClusterSize, Allocator>
::construct(Args&&... args) {
    value_type* p = this->allocate();
    new (p) value_type(std::forward<Args>(args)...);
    return p;
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
typename unordered_object_pool<ValueType, ClusterSize, Allocator>::value_type* unordered_object_pool<ValueType, ClusterSize, Allocator>
::allocate() {
    auto deploy_block = [](block_type* blk, cluster_node* key) -> value_type* {
        blk->key = key;
        return &blk->value;
    };

    // Try to allocate a block from the current cluster
    {
        block_type* blk = static_cast<block_type*>(_curr->cluster.allocate());
        if (ASH_LIKELY(blk != nullptr))
            return deploy_block(blk, _curr);
    }

    // Try to pop a free cluster from the stack
    {
        if (ASH_LIKELY(!_node_stack.empty())) {
            cluster_node* node = _node_stack.top();
            _node_stack.pop();
            assert(node->stacked == true);
            node->stacked = false;
            _insert_back_to(_curr, node);
            _curr = node;
            block_type* blk = static_cast<block_type*>(_curr->cluster.allocate());
            assert(blk != nullptr);
            return deploy_block(blk, _curr);
        }
    }

    // Allocate a new cluster
    assert(_curr->next == nullptr);
    cluster_node* node = _allocate_node();
    _insert_back_to(_curr, node);
    _curr = node;
    block_type* blk = static_cast<block_type*>(_curr->cluster.allocate());
    assert(blk != nullptr);
    return deploy_block(blk, _curr);
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
typename unordered_object_pool<ValueType, ClusterSize, Allocator>::value_type* unordered_object_pool<ValueType, ClusterSize, Allocator>
::allocate_zero_initialized() {
    value_type* p = this->allocate();
    memset(p, 0, sizeof(value_type));
    return p;
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
void unordered_object_pool<ValueType, ClusterSize, Allocator>::destroy(value_type* p) {
    p->~value_type();
    this->deallocate(p);
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
void unordered_object_pool<ValueType, ClusterSize, Allocator>::deallocate(value_type* p) {
    block_type* blk = reinterpret_cast<block_type*>(reinterpret_cast<char*>(p) - sizeof(void*));
    cluster_node* node = static_cast<cluster_node*>(blk->key);
    node->cluster.deallocate(blk);
    if (node == _curr)
        return;
    if (node->stacked)
        return;
    if (node->cluster.fill_rate() >= recycle_factor) {
        _detach_node(node);
        node->stacked = true;
        _node_stack.push(node);
    }
}

template <typename ValueType, size_t ClusterSize, typename Allocator>
void unordered_object_pool<ValueType, ClusterSize, Allocator>::reserve(size_t required) {
    if (required <= _capacity)
        return;
    size_t remained = roundup(required, cluster_size) - _capacity;
    assert(remained > 0);
    assert(remained % cluster_size == 0);
    remained /= cluster_size;
    for (size_t i = 0; i < remained; ++i) {
        cluster_node* node = _allocate_node();
        node->stacked = true;
        _node_stack.push(node);
    }
    _capacity += remained * cluster_size;
}

} // !namespace mixx
#endif // ASH_MEMORY_UNORDERED_OBJECT_POOL_H
