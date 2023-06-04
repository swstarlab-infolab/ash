#ifndef ASH_UNBOUNDED_CHANNEL_H
#define ASH_UNBOUNDED_CHANNEL_H
#include <ash/utility/dbg_log.h>
#include <boost/fiber/buffered_channel.hpp>
#include <deque>

namespace ash {

// unbounded, MPMC channel
template <typename T>
class unbounded_channel {
public:
    using value_type        = T;
    using channel_op_status = ::boost::fibers::channel_op_status;
    using _sync_chan_type   = ::boost::fibers::buffered_channel<int>;
    using _spinlock_t       = ::boost::fibers::detail::spinlock;
    using _que_type         = ::std::deque<value_type>;
    static constexpr int MaxNumRecvThreads = 64;

    unbounded_channel():
        _sync_chan(MaxNumRecvThreads) {
    }

    ~unbounded_channel() noexcept {
        if (!is_closed())
            close();
    }

    void close() noexcept {
        _sync_chan.close();
        _splk.lock();
        _que.clear();
        _splk.unlock();
    }

    bool is_closed() const noexcept {
        return _sync_chan.is_closed();
    }

    bool init() noexcept {
        try {
            return true;
        }
        catch (std::exception const& ex) {
            ASH_DMESG("An exception occured while initialize unbounded_channel! (%s)", ex.what());
        }
        catch (...) {
            ASH_DMESG("An exception occured while initialize unbounded_channel! (unknown error)");
        }
        close();
        return false;
    }

    channel_op_status push(value_type const& value) {
        return _push(value);
    }

    channel_op_status push(value_type&& value) {
        return _push(std::move(value));
    }

    channel_op_status pop(T& ret) {
        while (!is_closed()) {
            _splk.lock();
            if (_que.empty()) {
                _splk.unlock();
                if (!_wait())
                    break;
                continue;
            }
            ret = std::move(_que.front());
            _que.pop_front();
            _splk.unlock();
            return channel_op_status::success;
        }
        return channel_op_status::closed;
    }

private:
    template <typename T2>
    channel_op_status _push(T2&& value) {
        _splk.lock();
        _que.emplace_back(std::forward<T2>(value));
        if (_que.size() == 1) {
            if (!_wakeup())
                return channel_op_status::closed;
        }
        _splk.unlock();
        return channel_op_status::success;
    }

    bool _wait() {
        int _unused;
        channel_op_status const r = _sync_chan.pop(_unused);
        if (r == channel_op_status::success)
            return true;
        if (r == channel_op_status::closed)
            return false;
        ASH_FATAL("Invalid channel_op_staus");
    }

    bool _wakeup() {
        channel_op_status const r = _sync_chan.push(1);
        if (r == channel_op_status::success)
            return true;
        if (r == channel_op_status::closed)
            return false;
        ASH_FATAL("Invalid channel_op_staus");
    }

    _sync_chan_type _sync_chan;
    _que_type   _que;
    _spinlock_t _splk;
};

} // namespace ash

#endif // ASH_UNBOUNDED_CHANNEL_H
