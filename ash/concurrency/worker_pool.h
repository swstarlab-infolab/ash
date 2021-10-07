#ifndef ASH_CONCURRENCY_WORKER_POOL_H
#define ASH_CONCURRENCY_WORKER_POOL_H
#include <ash/concurrency/worker_thread.h>
#include <boost/lockfree/stack.hpp>
#include <atomic>

namespace ash {

template <typename FTy>
class worker_pool : noncopyable {
public:
    using worker_t = worker_thread<FTy>;
    worker_pool(size_t reserved = 1);
    ~worker_pool() noexcept;
    worker_t* allocate();
    void free(worker_t*);

private:
    boost::lockfree::stack<worker_t*> _free_list;
    std::atomic<size_t> _alloc_count;
    std::atomic<size_t> _size;
};

template <typename FTy>
worker_pool<FTy>::worker_pool(size_t const reserved) {
    _alloc_count = 0;
    _size = reserved;
    for (size_t i = 0; i < _size; ++i) {
        _free_list.push(new worker_t{ DefaultChannelSize });
    }
}

template <typename FTy>
worker_pool<FTy>::~worker_pool() noexcept {
    assert(_alloc_count == 0);
    size_t remained = _size;
    while (remained > 0) {
        worker_t* worker;
        while (!_free_list.pop(worker)) {}
        worker->join();
        delete worker;
        remained -= 1;
    }
}

template <typename FTy>
typename worker_pool<FTy>::worker_t* worker_pool<FTy>::allocate() {
    worker_t* worker;
    do {
        if (_free_list.pop(worker) == true)
            break;
        // Failed to allocate the worker from the free list!
        worker = new worker_t{ DefaultChannelSize };
        _size += 1;
    } while (false);

    _alloc_count += 1;
    return worker;
}

template <typename FTy>
void worker_pool<FTy>::free(worker_t* p) {
    assert(p->is_open() == true);
    _free_list.push(p);
    _alloc_count -= 1;
}

} // !namespace ash

#endif // ASH_CONCURRENCY_WORKER_POOL_H
