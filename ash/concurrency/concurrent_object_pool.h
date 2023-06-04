#ifndef ASH_CONCURRENCY_CONCURRENT_OBJECT_POOL_H
#define ASH_CONCURRENCY_CONCURRENT_OBJECT_POOL_H
#include <boost/pool/object_pool.hpp>
#include <ash/memory/unordered_object_pool.h>

namespace ash {

template <typename T,
    typename Mutex = std::mutex,
    typename UserAllocator = boost::default_user_allocator_new_delete >
class concurrent_object_pool {
public:
    T* malloc() {
        std::lock_guard<Mutex> g(_m);
        return _pool.malloc();
    }

    void free(T* p) {
        std::lock_guard<Mutex> g(_m);
        _pool.free(p);
    }

    template <typename ...Args>
    T* construct(Args&& ...args) {
        std::lock_guard<Mutex> g(_m);
        return _pool.construct(std::forward<Args>(args)...);
    }

    void destroy(T* p) {
        std::lock_guard<Mutex> g(_m);
        _pool.destroy(p);
    }

private:
    Mutex _m;
    boost::object_pool<T, UserAllocator> _pool;
};

template <typename T,
    size_t ClusterSize = 1024,
    typename Mutex = std::mutex,
    typename UserAllocator = boost::default_user_allocator_new_delete >
    class concurrent_unordered_object_pool {
public:
    T* malloc() {
        std::lock_guard<Mutex> g(_m);
        return _pool.allocate();
    }

    void free(T* p) {
        std::lock_guard<Mutex> g(_m);
        _pool.deallocate(p);
    }

    template <typename ...Args>
    T* construct(Args&& ...args) {
        std::lock_guard<Mutex> g(_m);
        return _pool.construct(std::forward<Args>(args)...);
    }

    void destroy(T* p) {
        std::lock_guard<Mutex> g(_m);
        _pool.destroy(p);
    }

private:
    Mutex _m;
    unordered_object_pool<T, ClusterSize> _pool;
};

} // !namespace ash
#endif // ASH_CONCURRENCY_CONCURRENT_OBJECT_POOL_H
