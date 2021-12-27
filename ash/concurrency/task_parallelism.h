#ifndef ASH_CONCURRENCY_TASK_PARALLELISM_H
#define ASH_CONCURRENCY_TASK_PARALLELISM_H
#include <ash/concurrency/worker_thread.h>
#include <ash/size.h>
#include <boost/fiber/unbuffered_channel.hpp>
#include <stack>

namespace ash {

template <typename FTy>
class async_task_executor {
public:
    using worker_t = worker_thread<FTy>;
    using pkg_task_t = typename worker_t::pkg_task_t;
    using callable_t = typename pkg_task_t::callable_t;
    using task_t = typename worker_t::task_t;

    async_task_executor(size_t chan_size, size_t num_workers = 8);
    ~async_task_executor() noexcept;
    void close();

    template <typename ...Args2>
    bool post(callable_t const& f, Args2&& ...args);

    template <typename ...Args2>
    bool post(callable_t&& f, Args2&& ...args);

    template <typename ...Args2>
    bool try_post(callable_t const& f, Args2&& ...args);

    template <typename ...Args2>
    bool try_post(callable_t&& f, Args2&& ...args);

    bool is_open() const;

    bool join();

    size_t const channel_size;
    size_t const worker_count;
private:
    using ichannel_t = boost::fibers::buffered_channel<task_t>;
    using rchannel_t = boost::fibers::buffered_channel<worker_t*>;
    using jchannel_t = boost::fibers::unbuffered_channel<bool>;

    template <typename Callable, typename ...Args2>
    bool _push(Callable&& f, Args2&& ...args);
    bool _push(task_t const& task);
    bool _push(task_t&& task);

    template <typename Callable, typename ...Args2>
    bool _try_push(Callable&& f, Args2&& ...args);
    bool _try_push(task_t const& task);
    bool _try_push(task_t&& task);

    bool _broadcast(task_t const& task);

    void _worker_callback(worker_t* worker, task_t* task);
    worker_t* _get_idle_worker();
    void _tmain();

    ichannel_t _ichan;
    rchannel_t _rchan;
    jchannel_t _jchan;
    worker_t*  _workers;
    std::stack<worker_t*> _free_list;
    std::thread _loop_thread;
};

template <typename FTy>
async_task_executor<FTy>::async_task_executor(size_t const chan_size, size_t const num_workers):
    channel_size(chan_size),
    worker_count(num_workers),
    _ichan(channel_size),
    _rchan(roundup2(worker_count + 3)) {
    _workers = new worker_t[worker_count];
    for (size_t i = 0; i < worker_count; ++i) {
        _workers[i].add_callback(
            std::bind(
                &async_task_executor::_worker_callback,
                this, 
                std::placeholders::_1, std::placeholders::_2
            )
        );
        _free_list.push(&_workers[i]);
    }
    _loop_thread = std::thread{ &async_task_executor::_tmain, this };
}

template <typename FTy>
async_task_executor<FTy>::~async_task_executor() noexcept {
    if (is_open())
        close();
    _loop_thread.join();
    delete[] _workers;
}

template <typename FTy>
void async_task_executor<FTy>::close() {
    assert(is_open());
    _ichan.close();
    _rchan.close();
    for (size_t i = 0; i < worker_count; ++i)
        _workers[i].close();
}

template <typename FTy>
template <typename ... Args2>
bool async_task_executor<FTy>::post(callable_t const& f, Args2&&... args) {
    return _push(f, std::forward<Args2>(args)...);
}

template <typename FTy>
template <typename ... Args2>
bool async_task_executor<FTy>::post(callable_t&& f, Args2&&... args) {
    return _push(std::move(f), std::forward<Args2>(args)...);
}

template <typename FTy>
template <typename ... Args2>
bool async_task_executor<FTy>::try_post(callable_t const& f, Args2&&... args) {
    return _push(f, std::forward<Args2>(args)...);
}

template <typename FTy>
template <typename ... Args2>
bool async_task_executor<FTy>::try_post(callable_t&& f, Args2&&... args) {
    return _push(std::move(f), std::forward<Args2>(args)...);
}

template <typename FTy>
bool async_task_executor<FTy>::is_open() const {
    return !_ichan.is_closed();
}

template <typename FTy>
bool async_task_executor<FTy>::join() {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    assert(is_open());
    task_t task;
    task.code = task_code::control;
    task.ct.ctlfunc = [](void* pthis_) {
        async_task_executor* pthis = reinterpret_cast<async_task_executor*>(pthis_);
        pthis->_jchan.push(true);
    };
    task.ct.ctlarg = this;
    if (_push(std::move(task))) {
        bool dummy;
        size_t join_count = 0;
        for (size_t i = 0; i < worker_count; ++i) {
            if (_jchan.pop(dummy) == channel_op_status::success)
                join_count += 1;
        }
        return join_count == worker_count;
    }
    return false;
}

template <typename FTy>
template <typename Callable, typename ... Args2>
bool async_task_executor<FTy>::_push(Callable&& f, Args2&&... args) {
    using namespace _worker_impl;
    task_t task;
    task.code = task_code::regular_task;
    task.pt = typename task_t::pkg_task_t{ std::forward<Callable>(f), { std::forward<Args2>(args)... } };
    return _push(std::move(task));
}

template <typename FTy>
bool async_task_executor<FTy>::_push(task_t const& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_ichan.push(task) == channel_op_status::success))
        return true;
    return false;
}

template <typename FTy>
bool async_task_executor<FTy>::_push(task_t&& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_ichan.push(std::move(task)) == channel_op_status::success))
        return true;
    return false;
}

template <typename FTy>
template <typename Callable, typename ... Args2>
bool async_task_executor<FTy>::_try_push(Callable&& f, Args2&&... args) {
    using namespace _worker_impl;
    return _try_push(
        task_t{
           task_code::regular_task,
           std::forward<Callable>(f),
           { std::forward<Args2>(args)... }
        }
    );
}

template <typename FTy>
bool async_task_executor<FTy>::_try_push(task_t const& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_ichan.try_push(task) == channel_op_status::success))
        return true;
    return false;
}

template <typename FTy>
bool async_task_executor<FTy>::_try_push(task_t&& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_ichan.try_push(std::move(task)) == channel_op_status::success))
        return true;
    return false;
}

template <typename FTy>
bool async_task_executor<FTy>::_broadcast(task_t const& task) {
    size_t c = 0;
    for (size_t i = 0; i < worker_count; ++i) {
        if (_workers[i].push(task))
            c += 1;
    }
    return c == worker_count;
}

template <typename FTy>
void async_task_executor<FTy>::_worker_callback(worker_t* worker, task_t* task) {
    using namespace _worker_impl;
    if (task->code == task_code::regular_task)
        _rchan.push(worker);
}

template <typename FTy>
typename async_task_executor<FTy>::worker_t* async_task_executor<FTy>::_get_idle_worker() {
    using namespace boost;
    using namespace fibers;
    worker_t* worker;
    if (!_free_list.empty()) {
        worker = _free_list.top();
        _free_list.pop();
        return worker;
    }

    while (_rchan.try_pop(worker) == channel_op_status::success)
        _free_list.push(worker);

    if (!_free_list.empty()) {
        assert(_free_list.top() == worker);
        _free_list.pop();
        return worker;
    }

    if (_rchan.pop(worker) == channel_op_status::success)
        return worker;

    return nullptr;
}

template <typename FTy>
void async_task_executor<FTy>::_tmain() {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    task_t task;
    do {
        channel_op_status const status = _ichan.pop(task);
        if (ASH_LIKELY(status == channel_op_status::success)) {
            if (task.code == task_code::regular_task) {
                worker_t* worker = _get_idle_worker();
                if (ASH_UNLIKELY(worker == nullptr))
                    break; // the async_task_executor may have been closed...
                bool const r = worker->push(std::move(task));
                assert(r);
                _ash_unused(r);
            }
            else if (task.code == task_code::control) {
                bool r = _broadcast(task);
                assert(r);
                _ash_unused(r);
            }
        }
        else if (status == channel_op_status::closed) {
            break;
        }
        else {
            fprintf(stderr, "Invalid protocol error from %s\n", __FUNCTION__);
            fflush(stderr);
            std::exit(-1);
        }
    } while (true);
}

} // !namespace ash
#endif // ASH_CONCURRENCY_TASK_PARALLELISM_H
