#ifndef ASH_CONCURRENCY_FRAMEWORK_MESSAGE_LOOP_H
#define ASH_CONCURRENCY_FRAMEWORK_MESSAGE_LOOP_H
#include <ash/concurrency/detail/message_passing_internals.h>
#include <boost/fiber/buffered_channel.hpp>
#include <ash/numeric.h>
#include <ash/detail/noncopyable.h>
#include <ash/utility/dbg_log.h>
#include <thread>
#include <string>
#include <functional>

namespace ash {

/* Generic message loop framework for event-driven design */

template <typename MsgTy>
class channel_based_message_broker: noncopyable {
public:
    using retcode_t = message_framework_return_code;
    using message_t  = MsgTy;
    using channel_t  = boost::fibers::buffered_channel<message_t*>;
    using msgproc_t  = std::function<void(message_t*)>;
    using inithook_t = std::function<void(void*/* inithook_attr */)>;
    using exithook_t = std::function<void(retcode_t, void*/* exithook_attr */)>;
    using channel_op_status = boost::fibers::channel_op_status;

    struct config_t {
        uint64_t    channel_size  = 0;
        msgproc_t   msgproc       = nullptr;
        inithook_t  inithook      = nullptr;
        void*       inithook_attr = nullptr;
        exithook_t  exithook      = nullptr;
        void*       exithook_attr = nullptr;
        char const* name          = "noname";
    };

    channel_based_message_broker();
    ~channel_based_message_broker() noexcept;
    retcode_t init(config_t const& cfg) noexcept;
    bool close() noexcept;
    retcode_t send_message(message_t* m) noexcept;

    bool is_initialized() const noexcept {
        return _mchan != nullptr;
    }

    config_t const& config() const noexcept {
        return _cfg;
    }

    std::string const& name() const noexcept {
        return _desc;
    }

private:
    void _msgloop();

    config_t    _cfg;
    std::string _desc;
    channel_t*  _mchan;  // a channel for message passing
    std::thread _thread; // a background thread to provide callback processing
};

template <typename MsgTy>
channel_based_message_broker<MsgTy>::channel_based_message_broker() {
    _desc  = "null";
    _mchan = nullptr;
}

template <typename MsgTy>
channel_based_message_broker<MsgTy>::~channel_based_message_broker() noexcept {
    if (!is_initialized())
        return;
    if (!close()) {
        ASH_ERRLOG("Failed to close channel_based_message_broker <%s>", name().c_str());
    }
}


template <typename MsgTy>
typename channel_based_message_broker<MsgTy>::retcode_t
channel_based_message_broker<MsgTy>::init(config_t const& cfg) noexcept {
    try {
        _cfg = cfg;
        _desc = _cfg.name;
        if (_cfg.channel_size == 1)
            _cfg.channel_size = 2;
        size_t const actual_size = ash::roundup2(_cfg.channel_size);
        _mchan = new channel_t(actual_size);
        _thread = std::thread(&channel_based_message_broker::_msgloop, this);
        if (!_thread.joinable())
            return retcode_t::ThreadCreationError;
        return retcode_t::Success;
    }
    ASH_CLAUSE_CATCH_STDEXCEPT;
    return retcode_t::UnhandledException;
}

template <typename MsgTy>
bool channel_based_message_broker<MsgTy>::close() noexcept {
    try {
        if (is_initialized()) {
            _mchan->close();
            _thread.join();
            delete _mchan; _mchan = nullptr;
            ASH_DMESG("message broker <%s> is closed", name().c_str());
        }
        return true;
    }
    ASH_CLAUSE_CATCH_STDEXCEPT;
    ASH_ERRLOG("message broker <%s> is closed with unhandled exception!", name().c_str());
    return false;
}

template <typename MsgTy>
typename channel_based_message_broker<MsgTy>::retcode_t
channel_based_message_broker<MsgTy>::send_message(message_t* m) noexcept {
    try {
        switch (_mchan->try_push(m)) {
        case channel_op_status::success: 
            return retcode_t::Success;
        case channel_op_status::empty: 
            return retcode_t::ChannelEmpty;
        case channel_op_status::full:
            return retcode_t::ChannelFull;
        case channel_op_status::closed:
            return retcode_t::ChannelClosed;
        case channel_op_status::timeout:
            return retcode_t::ChannelTimeout;
        default: // NOLINT(clang-diagnostic-covered-switch-default)
            throw std::logic_error("Invalid channel status detected; Check the version of boost::fibers library!");
        }
    }
    ASH_CLAUSE_CATCH_STDEXCEPT;
    return retcode_t::UnhandledException;
}

template <typename MsgTy>
void channel_based_message_broker<MsgTy>::_msgloop() {
    using namespace boost;
    using namespace fibers;

    if (_cfg.inithook != nullptr)
        _cfg.inithook(_cfg.inithook_attr);

    retcode_t exit_state = retcode_t::Undefined;

    try {
        do {
            message_t* m;
            channel_op_status const r = _mchan->pop(m);

            if (r == channel_op_status::success) {
                _cfg.msgproc(m);
                continue;
            }

            if (r == channel_op_status::closed) {
                exit_state = retcode_t::ChannelClosed;
                break;
            }

            // Abnormal cases
            switch (r) {
            case channel_op_status::empty:
                exit_state = retcode_t::ChannelEmpty;
                break;
            case channel_op_status::full:
                exit_state = retcode_t::ChannelFull;
                break;
            case channel_op_status::timeout:
                exit_state = retcode_t::ChannelTimeout;
                break;
            default: // NOLINT(clang-diagnostic-covered-switch-default)
                exit_state = retcode_t::Undefined;
            }
            throw std::logic_error("Invalid channel status detected; Check the version of boost::fibers library!");

        } while (true);
    }
    ASH_CLAUSE_CATCH_STDEXCEPT;

    if (_cfg.exithook != nullptr)
        _cfg.exithook(exit_state, _cfg.exithook_attr);
}

namespace transmission_policy {

/********************************************
 * Transmission policy: SYNCHRONOUS (Default)
 ********************************************/

template <typename MsgTy>
class synchronous {
public:
    using message_t = MsgTy;
    using msgbroker_t = channel_based_message_broker<message_t>;

    static constexpr bool is_async = false;

    struct config_t {
        msgbroker_t* broker = nullptr;
    };

    synchronous() {
    }

    ~synchronous() noexcept {
        stop();
    }

    message_framework_return_code run(config_t const& cfg) noexcept {
        _cfg = cfg;
        return message_framework_return_code::Success;
    }

    [[maybe_unused]] static message_framework_return_code stop() noexcept {
        // nothing to do here.
        return message_framework_return_code::Success;
    }

    message_framework_return_code post(message_t* m) noexcept {
        return _cfg.broker->send_message(m);
    }

private:
    config_t _cfg;
};

/********************************************
 * Transmission policy: ASYNCHRONOUS
 ********************************************/

template <typename MsgTy>
class asynchronous {
public:
    using message_t = MsgTy;
    using channel_t  = boost::fibers::buffered_channel<message_t*>;
    using msgbroker_t = channel_based_message_broker<message_t>;

    static constexpr bool is_async = true;

    struct config_t {
        msgbroker_t* broker;
    };

public:
    asynchronous(): _chan(32) {
        _cfg.broker = nullptr;
        _mchain_size = 0;
    }

    ~asynchronous() noexcept {
        message_framework_return_code const r = stop();
        if (r != message_framework_return_code::Success)
            ASH_ERRLOG("Unexpected error occured at destructor!");
        
    }

    [[nodiscard]] message_framework_return_code run(config_t const& cfg) noexcept {
        try {
            _cfg = cfg;
            _thread = std::thread(&asynchronous::_msgloop, this);
            if (!_thread.joinable())
                return message_framework_return_code::ThreadCreationError;
            return message_framework_return_code::Success;
        }
        ASH_CLAUSE_CATCH_STDEXCEPT;
        return message_framework_return_code::UnhandledException;
    }

    [[nodiscard]] message_framework_return_code stop() noexcept {
        try {
            if (is_initialized()) {
                _chan.close();
                _thread.join();
            }
            return message_framework_return_code::Success;
        }
        ASH_CLAUSE_CATCH_STDEXCEPT;
        return message_framework_return_code::UnhandledException;
    }

    [[nodiscard]] message_framework_return_code post(message_t* m) noexcept {
        using namespace boost;
        using namespace fibers;

        try {
            switch (_chan.push(m)) {
            case channel_op_status::success:
                return message_framework_return_code::Success;
            case channel_op_status::empty:
                return message_framework_return_code::ChannelEmpty;
            case channel_op_status::full:
                return message_framework_return_code::ChannelFull;
            case channel_op_status::closed:
                return message_framework_return_code::ChannelClosed;
            case channel_op_status::timeout:
                return message_framework_return_code::ChannelTimeout;
            default: // NOLINT(clang-diagnostic-covered-switch-default)
                throw std::logic_error("Invalid channel status detected; Check the version of boost::fibers library!");
            }
        }
        ASH_CLAUSE_CATCH_STDEXCEPT;
        return message_framework_return_code::UnhandledException;
    }

    [[nodiscard]] bool is_initialized() const noexcept {
        return _thread.joinable();
    }

private:
    using mchain_t = list_builder<mchain_node::_mchain_control_block>;
    config_t    _cfg;
    channel_t   _chan; // a channel for message relaying
    mchain_t    _mchain;
    uint64_t    _mchain_size;
    std::thread _thread; // a background thread to relay messages

    void _msgloop() noexcept {
        using namespace boost;
        using namespace fibers;

        message_framework_return_code exit_state = message_framework_return_code::Undefined;
        try {
            do {
                message_t* m;
                channel_op_status r;

                if (_mchain_size > 0)
                    r = _chan.try_pop(m);
                else
                    r = _chan.pop(m);

                switch (r) {
                case channel_op_status::closed:
                    exit_state = message_framework_return_code::ChannelClosed;
                    continue;
                case channel_op_status::success:
                    _enqueue_message_to_mchain(m);
                    [[fallthrough]];
                case channel_op_status::empty:
                    assert(_mchain_size > 0);
                    exit_state = _relay_messages();
                    if (exit_state != message_framework_return_code::Success)
                        throw std::runtime_error("Relay operation failure!");
                    continue; // move to next loop

                // Abnormal cases
                case channel_op_status::full:
                    exit_state = message_framework_return_code::ChannelFull;
                    break;
                case channel_op_status::timeout:
                    exit_state = message_framework_return_code::ChannelTimeout;
                    break;
                default: // NOLINT(clang-diagnostic-covered-switch-default)
                    exit_state = message_framework_return_code::Undefined;
                }
                throw std::logic_error("Invalid channel status detected; Check the version of boost::fibers library!");

            } while (exit_state != message_framework_return_code::ChannelClosed);
        }
        ASH_CLAUSE_CATCH_STDEXCEPT;
        ASH_DMESG("message broker <%s>'s transmission_policy is closed with code %d (%s)", _cfg.broker->name().c_str(), static_cast<int>(exit_state), exit_state.to_string());
    }

    void _enqueue_message_to_mchain(mchain_node* m) noexcept {
        m->clear_mchain_properties();
        _mchain.insert_back(&m->_mchain_ctl);
        _mchain_size += 1;
        if (_mchain_size > 1) {
            m->set_mchain_flags(MChainFlag_Pending);
            if (m->_mchain_ctl.callback)
                m->_mchain_ctl.callback(static_cast<message_t*>(m), message_framework_return_code::Pending);
        }
    }

    [[nodiscard]] message_framework_return_code _relay_messages() noexcept {
        while (!_mchain.empty()) {
            mchain_t::iterator it = _mchain.begin();
            message_t* m = reinterpret_cast<message_t*>(it.get_target_pointer());
            _mchain.remove_node(it);
            message_framework_return_code const ec = 
                _cfg.broker->send_message(reinterpret_cast<message_t*>(m));

            if (ec == message_framework_return_code::Success) {
                _mchain_size -= 1;
                if (m->_mchain_ctl.callback)
                    m->_mchain_ctl.callback(static_cast<message_t*>(m), message_framework_return_code::Success);
            }
            else if (ec == message_framework_return_code::ChannelFull) {
                _mchain.insert_front(&m->_mchain_ctl);
                if (!m->test_mchain_flags(MChainFlag_Pending)) {
                    m->set_mchain_flags(MChainFlag_Pending);
                    if (m->_mchain_ctl.callback)
                        m->_mchain_ctl.callback(static_cast<message_t*>(m), message_framework_return_code::Pending);
                    //assert(it.get_target_pointer()->get_next_node() == nullptr);
                }
                else std::this_thread::yield();
                break;
            }
            else {
                ASH_ERRLOG("%s's transmission_policy is failed to relay messages!");
                if (m->_mchain_ctl.callback)
                    m->_mchain_ctl.callback(static_cast<message_t*>(m), message_framework_return_code::RelayError);
                return message_framework_return_code::RelayError;
            }
        }
        return message_framework_return_code::Success;
    }
};

} // namespace transmission_policy

template <typename MsgTy,
          template <typename M> class TsmPolicy = transmission_policy::synchronous >
class message_passing_framework: noncopyable {
public:
    using message_t = MsgTy;
    using _msgbroker_t = channel_based_message_broker<message_t>;
    using _tsm_policy = TsmPolicy<message_t>;
    using error_code = message_framework_return_code;

    struct msgfw_config_t: _msgbroker_t::config_t {
    };

    message_passing_framework() {
    }

    virtual ~message_passing_framework() noexcept {
        _msgfw.stop_message_framework();
    }

    [[nodiscard]] message_framework_return_code post(message_t* m) {
        return _msgfw.policy.post(m);
    }

protected:
    struct {
        msgfw_config_t cfg;
        _msgbroker_t broker;
        _tsm_policy  policy;

        bool is_initialized() const noexcept {
            return broker.is_initialized();
        }

        error_code run_message_framework(msgfw_config_t const& cfg_) {
            cfg = cfg_;

            // Init a message broker
            error_code exit_code = broker.init(cfg);
            if (exit_code != error_code::Success)
                goto lb_error;

            // Init a transmission policy
            {
                typename _tsm_policy::config_t tsm_cfg;
                tsm_cfg.broker = &broker; // framework_config
                exit_code = policy.run(tsm_cfg);
                if (exit_code != error_code::Success)
                    goto lb_error;
            }

            return error_code::Success;

        lb_error:
            stop_message_framework();
            return exit_code;
        }

        void stop_message_framework() noexcept {
            //!IMPORTANT: transmission policy must be closed before the message broker
            if (policy.stop() != error_code::Success)
                ASH_ERRLOG("An error occured when closing the transmission policy");
            if (!broker.close())
                ASH_ERRLOG("An error occured when closing the message broker");
        }
    } _msgfw;
};

template <typename MsgTy>
using async_message_passing_framework = 
    message_passing_framework<MsgTy, transmission_policy::asynchronous>;

} // namespace ash

#endif // !ASH_CONCURRENCY_FRAMEWORK_MESSAGE_LOOP_H
