#ifndef ASH_CONCURRENCY_WORKER_THREAD
#define ASH_CONCURRENCY_WORKER_THREAD
#include <ash/detail/noncopyable.h>
#include <ash/config.h>
#include <ash/mpl/sequence_generator.h>
#include <boost/fiber/buffered_channel.hpp>
#include <tuple>
#include <functional>

namespace ash {

namespace _worker_impl {

enum class task_code {
    undefined,
    regular_task,
    control,
};

namespace task_type {

ASH_DECLARE_SYMBOL(regular);
ASH_DECLARE_SYMBOL(join);
ASH_DECLARE_SYMBOL(control);

} // namespace task_type

template <typename FTy>
struct packaged_task;

template <typename RTy>
struct packaged_task<RTy()> {
    using callable_t = std::function<RTy()>;
    using ctlfunc_t = std::function<void(void*)>;
    using pkg_args_t = void;
    callable_t callable;
    ctlfunc_t ctlfunc;
};

template <typename RTy, typename ...Args>
struct packaged_task<RTy(Args...)> {
    using callable_t = std::function<RTy(Args...)>;
    using pkg_args_t = std::tuple<Args...>;
    callable_t callable;
    pkg_args_t args;
};

struct control_task {
    using ctlfunc_t = std::function<void(void*)>;
    ctlfunc_t ctlfunc;
    void* ctlarg = nullptr;
};

template <typename FTy>
struct worker_task {
    using pkg_task_t = packaged_task<FTy>;
    task_code code = task_code::undefined;
    pkg_task_t pt;
    control_task ct;
};

} // !namespace _worker_impl

template <typename FTy>
class worker_thread;

template <typename RTy, typename ...Args>
class worker_thread<RTy(Args...)> : noncopyable {
public:
    using task_t = _worker_impl::worker_task<RTy(Args...)>;
    using pkg_task_t = typename task_t::pkg_task_t;
    using callable_t = typename pkg_task_t::callable_t;
    using pkg_args_t = typename pkg_task_t::pkg_args_t;
    using channel_t = boost::fibers::buffered_channel<task_t>;
    using callback_t = std::function<void(worker_thread* /*worker*/, task_t* /*task*/)>;
    explicit worker_thread(size_t chan_size = DefaultChannelSize);
    ~worker_thread() noexcept;
    void close();

    void add_callback(callback_t const& callback);

    template <typename ...Args2>
    bool push(callable_t const& f, Args2&& ...args);

    template <typename ...Args2>
    bool push(callable_t&& f, Args2&& ...args);

    bool push(task_t const&);
    bool push(task_t&&);

    template <typename ...Args2>
    bool try_push(callable_t const& f, Args2&& ...args);

    template <typename ...Args2>
    bool try_push(callable_t&& f, Args2&& ...args);

    bool try_push(task_t const&);
    bool try_push(task_t&&);

    bool is_open() const;

    bool join();

private:
    static constexpr unsigned argc = std::tuple_size<pkg_args_t>::value;
    using sequence_t = typename mpl::sequence_generator<argc>::type;
    using join_channel_t = boost::fibers::buffered_channel<int>;

    template <typename Callable, typename ...Args2>
    bool _push(Callable&& f, Args2&& ...args);
    bool _push(task_t const& task);
    bool _push(task_t&& task);

    template <typename Callable, typename ...Args2>
    bool _try_push(Callable&& f, Args2&& ...args);
    bool _try_push(task_t const& task);
    bool _try_push(task_t&& task);

    void _message_loop();
    template <int ...S>
    void _unpack(task_t const& task, mpl::sequence_type<S...>);

    channel_t _chan;
    join_channel_t _join_chan;
    std::thread _thread;
    callback_t _callback;
};

template <typename RTy, typename ... Args>
worker_thread<RTy(Args...)>::worker_thread(size_t chan_size) :
    _chan(chan_size), _join_chan(4) {
    _thread = std::thread{ &worker_thread::_message_loop, this };
    _callback = nullptr;
}

template <typename RTy, typename ... Args>
worker_thread<RTy(Args...)>::~worker_thread() noexcept {
    if (is_open()) {
        close();
    }
}

template <typename RTy, typename ... Args>
void worker_thread<RTy(Args...)>::close() {

    try {
        _chan.close();
        _thread.join();
    }
    catch (std::system_error const& ex) {
        fprintf(stderr, "FATAL ERROR from %s; An exception <std::system_error> is occured! (detail: %s)", __FUNCTION__, ex.what());
        fflush(stderr);
        std::exit(-1);
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "FATAL ERROR from %s; An exception <std::exception> is occured! (detail: %s)", __FUNCTION__, ex.what());
        fflush(stderr);
        std::exit(-1);
    }
    catch (...) {
        fprintf(stderr, "FATAL ERROR from %s; An exception is occured! (unknown exception)", __FUNCTION__);
        fflush(stderr);
        std::exit(-1);
    }
}

template <typename RTy, typename ... Args>
void worker_thread<RTy(Args...)>::add_callback(callback_t const& callback) {
    _callback = callback;
}

template <typename RTy, typename ... Args>
template <typename ... Args2>
bool worker_thread<RTy(Args...)>::push(callable_t const& f, Args2&&... args) {
    return _push(f, std::forward<Args2>(args)...);
}

template <typename RTy, typename ... Args>
template <typename ... Args2>
bool worker_thread<RTy(Args...)>::push(callable_t&& f, Args2&&... args) {
    return _push(std::move(f), std::forward<Args2>(args)...);
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::push(task_t const& task) {
    return _push(task);
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::push(task_t&& task) {
    return _push(std::move(task));
}

template <typename RTy, typename ... Args>
template <typename ... Args2>
bool worker_thread<RTy(Args...)>::try_push(callable_t const& f, Args2&&... args) {
    return _push(f, std::forward<Args2>(args)...);
}

template <typename RTy, typename ... Args>
template <typename ... Args2>
bool worker_thread<RTy(Args...)>::try_push(callable_t&& f, Args2&&... args) {
    return _push(std::move(f), std::forward<Args2>(args)...);
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::try_push(task_t const& task) {
    return _try_push(task);
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::try_push(task_t&& task) {
    return _try_push(std::move(task));
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::is_open() const {
    return !_chan.is_closed();
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::join() {
    using namespace _worker_impl;
    task_t task;
    task.code = task_code::control;
    task.ct.ctlfunc = [](void* pthis_) {
        worker_thread* pthis = reinterpret_cast<worker_thread*>(pthis_);
        pthis->_join_chan.push(1);
    };
    task.ct.ctlarg = this;
    if (_push(std::move(task))) {
        int dummy;
        _join_chan.pop(dummy);
        return true;
    }
    return false;
}

template <typename RTy, typename ... Args>
template <typename Callable, typename ... Args2>
bool worker_thread<RTy(Args...)>::_push(Callable&& f, Args2&&... args) {
    using namespace _worker_impl;
    task_t task;
    task.code = task_code::regular_task;
    task.pt = typename task_t::pkg_task_t{ std::forward<Callable>(f), { std::forward<Args2>(args)... } };
    return _push(std::move(task));
}

template<typename RTy, typename ...Args>
bool worker_thread<RTy(Args...)>::_push(task_t const& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_chan.push(task) == channel_op_status::success))
        return true;
    return false;
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::_push(task_t&& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_chan.push(std::move(task)) == channel_op_status::success))
        return true;
    return false;

}

template <typename RTy, typename ... Args>
template <typename Callable, typename ... Args2>
bool worker_thread<RTy(Args...)>::_try_push(Callable&& f, Args2&&... args) {
    using namespace _worker_impl;
    task_t task;
    task.code = task_code::regular_task;
    task.pt = typename task_t::pkg_task_t{ std::forward<Callable>(f), std::forward<Args2>(args)... };
    return _try_push(std::move(task));
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::_try_push(task_t const& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_chan.try_push(task) == channel_op_status::success))
        return true;
    return false;
}

template <typename RTy, typename ... Args>
bool worker_thread<RTy(Args...)>::_try_push(task_t&& task) {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    if (ASH_LIKELY(_chan.try_push(std::move(task)) == channel_op_status::success))
        return true;
    return false;
}

template <typename RTy, typename ... Args>
void worker_thread<RTy(Args...)>::_message_loop() {
    using namespace _worker_impl;
    using namespace boost;
    using namespace fibers;
    task_t task;
    do {
        channel_op_status const status = _chan.pop(task);
        if (ASH_LIKELY(status == channel_op_status::success)) {
            if (ASH_LIKELY(task.code == task_code::regular_task))
                _unpack(task, sequence_t{});
            else if (task.code == task_code::control) {
                task.ct.ctlfunc(task.ct.ctlarg);
            }
            if (_callback != nullptr)
                _callback(this, &task);
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

template <typename RTy, typename ... Args>
template <int... S>
void worker_thread<RTy(Args...)>::_unpack(task_t const& task, mpl::sequence_type<S...>) {
    task.pt.callable(std::get<S>(task.pt.args)...);
}

} // !namespace ash

#endif // ASH_CONCURRENCY_WORKER_THREAD
