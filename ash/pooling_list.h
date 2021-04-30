#ifndef _ASH_POOLING_LIST_H_
#define _ASH_POOLING_LIST_H_
#include <ash/detail/list_base.h>
#include <ash/detail/noncopyable.h>
#include <boost/pool/object_pool.hpp>

namespace ash {

template <typename T, size_t ClusterSize = 256>
using list_node_pool_trait = boost::object_pool< list_impl::list_node<T>>;

/* class pooling_list */

template <typename T, typename Pool = list_node_pool_trait<T> >
class pooling_list : noncopyable {
    friend class list_impl;
public:
    using node_type = list_impl::list_node<T>;
    using node_pointer = node_type*;
    using value_type = T;
    using pool_type = Pool;
    using iterator = list_impl::list_iterator<T>;

    explicit pooling_list(pool_type& pool);
    ~pooling_list() noexcept;
    template <typename ...Args> iterator emplace_front_of(iterator const& target, Args&& ...args);
    template <typename ...Args> iterator emplace_back_to(iterator const& target, Args&& ...args);
    template <typename ...Args> iterator emplace_front(Args&& ...args);
    template <typename ...Args> iterator emplace_back(Args&& ...args);
    iterator remove_node(iterator const& target);
    node_pointer remove_node(node_pointer const& target);
    void clear();

    ASH_FORCEINLINE iterator begin() const {
        return iterator{ _head->next };
    }

    ASH_FORCEINLINE iterator tail() const {
        return iterator{ _tail };
    }

    ASH_FORCEINLINE iterator end() const {
        return iterator{ nullptr };
    }

    ASH_FORCEINLINE bool empty() const {
        return _head->next == nullptr;
    }

    ASH_FORCEINLINE size_t size() const {
        return _size;
    }

    ASH_FORCEINLINE node_pointer head() const {
        return _head;
    }

private:
    template <typename ...Args>
    static ASH_FORCEINLINE void _init_value(node_type* node, Args ...args) {
        new (&node->value) value_type(std::forward<Args>(args)...);
    }

    static ASH_FORCEINLINE void _free_value(node_type* node) {
        node->value.~value_type();
    }

    node_type* _allocate_node();
    void _deallocate_node(node_type* node);

    pool_type& _pool;
    node_pointer _head, _tail;
    size_t _size;
};

template <typename T, typename PoolTy>
pooling_list<T, PoolTy>::pooling_list(pool_type& pool) : _pool(pool) {
    _tail = _head = _allocate_node();
    _head->prev = nullptr;
    _head->next = nullptr;
    _size = 0;
}

template <typename T, typename PoolTy>
pooling_list<T, PoolTy>::~pooling_list() noexcept {
    try {
        clear();
        _deallocate_node(_head);
    }
    catch (...) {
        fprintf(stderr, "Failed to cleanup pooling list!");
        std::abort();
    }
}

template <typename T, typename PoolTy>
template <typename ... Args>
typename pooling_list<T, PoolTy>::iterator pooling_list<T, PoolTy>::
emplace_front_of(iterator const& target, Args&&... args) {
    return list_impl::emplace_front_of(*this, target, std::forward<Args>(args)...);
}

template <typename T, typename PoolTy>
template <typename ... Args>
typename pooling_list<T, PoolTy>::iterator pooling_list<T, PoolTy>::
emplace_back_to(iterator const& target, Args&&... args) {
    return list_impl::emplace_back_to(*this, target, std::forward<Args>(args)...);
}

template <typename T, typename PoolTy>
template <typename ... Args>
typename pooling_list<T, PoolTy>::iterator pooling_list<T, PoolTy>::emplace_front(Args&&... args) {
    return emplace_back_to(iterator(_head), std::forward<Args>(args)...);
}

template <typename T, typename PoolTy>
template <typename ... Args>
typename pooling_list<T, PoolTy>::iterator pooling_list<T, PoolTy>::emplace_back(Args&&... args) {
    return emplace_back_to(iterator(_tail), std::forward<Args>(args)...);
}

template <typename T, typename PoolTy>
typename pooling_list<T, PoolTy>::iterator pooling_list<T, PoolTy>::remove_node(iterator const& target) {
    return list_impl::remove_node(*this, target);
}

template <typename T, typename Pool>
typename pooling_list<T, Pool>::node_pointer pooling_list<T, Pool>::remove_node(node_pointer const& target) {
    return list_impl::remove_node(*this, target);
}

template <typename T, typename PoolTy>
void pooling_list<T, PoolTy>::clear() {
    while (_head->next != nullptr)
        remove_node(iterator(_head->next));
}

template <typename T, typename PoolTy>
typename pooling_list<T, PoolTy>::node_type* pooling_list<T, PoolTy>::_allocate_node() {
    return _pool.malloc();
}

template <typename T, typename PoolTy>
void pooling_list<T, PoolTy>::_deallocate_node(node_type* node) {
    _pool.free(node);
}

/* class private_pooling_list */

template <typename ValueType, unsigned ClusterSize = 256, typename Allocator = std::allocator<ValueType>>
class private_pooling_list : noncopyable {
    using node_type = list_impl::list_node<ValueType>;
public:
    using value_type = ValueType;
    using alloc_type = Allocator;
    using iterator = list_impl::list_iterator<value_type>;
    constexpr static unsigned cluster_size = ClusterSize;
    explicit private_pooling_list(size_t pool_reserved = cluster_size, const alloc_type& alloc = alloc_type());
    ~private_pooling_list() noexcept;
    template <typename ...Args> iterator emplace_front_of(iterator target, Args&& ...args);
    template <typename ...Args> iterator emplace_back_to(iterator target, Args&& ...args);
    template <typename ...Args> iterator emplace_front(Args&& ...args);
    template <typename ...Args> iterator emplace_back(Args&& ...args);
    iterator remove_node(iterator target) noexcept;
    void reserve_pool(size_t size);
    void clear();

    ASH_FORCEINLINE iterator begin() const {
        return iterator(_head->next);
    }

    ASH_FORCEINLINE iterator tail() const {
        return iterator(_tail);
    }

    ASH_FORCEINLINE iterator end() const {
        return iterator(nullptr);
    }

    ASH_FORCEINLINE bool empty() const {
        return _head->next == nullptr;
    }

private:
    using alproxy_t = typename std::allocator_traits<alloc_type>::template rebind_alloc<node_type>;
    node_type* _head;
    node_type* _tail;
    size_t _size;
    boost::object_pool<node_type, alproxy_t> _pool;
    node_type* _allocate_node();
    void _deallocate_node(node_type* node) noexcept;

    template <typename ...Args>
    static ASH_FORCEINLINE void _construct_data(node_type* node, Args ...args) {
        new (&node->value) value_type(std::forward<Args>(args)...);
    }

    static ASH_FORCEINLINE void _destruct_data(node_type* node) {
        node->value.~value_type();
    }
};

template <typename ValueType, unsigned ClusterSize, typename Allocator>
private_pooling_list<ValueType, ClusterSize, Allocator>::private_pooling_list(size_t pool_reserved, const alloc_type& alloc) :
    _pool(pool_reserved, alloc) {
    node_type* head = _allocate_node();
    head->next = nullptr;
    head->prev = nullptr;
    _tail = _head = head;
    _size = 0;
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
private_pooling_list<ValueType, ClusterSize, Allocator>::~private_pooling_list() noexcept {
    clear();
    _deallocate_node(_head);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
template <typename ... Args>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::iterator
private_pooling_list<ValueType, ClusterSize, Allocator>::emplace_front_of(iterator target, Args&&... args) {
    return list_impl::emplace_front_of(*this, target, std::forward<Args>(args)...);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
template <typename ... Args>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::iterator
private_pooling_list<ValueType, ClusterSize, Allocator>::emplace_back_to(iterator target, Args&&... args) {
    return list_impl::emplace_back_to(*this, target, std::forward<Args>(args)...);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
template <typename ... Args>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::iterator
private_pooling_list<ValueType, ClusterSize, Allocator>::emplace_front(Args&&... args) {
    return emplace_back_to(iterator(_head), std::forward<Args>(args)...);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
template <typename ... Args>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::iterator
private_pooling_list<ValueType, ClusterSize, Allocator>::emplace_back(Args&&... args) {
    return emplace_back_to(iterator(_tail), std::forward<Args>(args)...);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::iterator
private_pooling_list<ValueType, ClusterSize, Allocator>::remove_node(iterator target) noexcept {
    return list_impl::remove_node(*this, target);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
void private_pooling_list<ValueType, ClusterSize, Allocator>::reserve_pool(size_t size) {
    _pool.reserve(size);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
void private_pooling_list<ValueType, ClusterSize, Allocator>::clear() {
    while (_head->next != nullptr)
        remove_node(iterator(_head->next));
    assert(_size == 0);
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
typename private_pooling_list<ValueType, ClusterSize, Allocator>::node_type* private_pooling_list<ValueType, ClusterSize, Allocator>::_allocate_node() {
    return _pool.allocate();
}

template <typename ValueType, unsigned ClusterSize, typename Allocator>
void private_pooling_list<ValueType, ClusterSize, Allocator>::_deallocate_node(node_type* node) noexcept {
    _pool.deallocate(node);
}

} // !namespace ash

#endif // _ASH_POOLING_LIST_H_
