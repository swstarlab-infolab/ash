#ifndef ASH_CONCURRENCY_DETAIL_MESSAGE_PASSING_INTERNALS_H
#define ASH_CONCURRENCY_DETAIL_MESSAGE_PASSING_INTERNALS_H
#include <ash/list_builder.h>

namespace ash {

struct message_framework_return_code {
    enum {
        Undefined = 0,
        Success,
        Pending,
        InvalidChannelSize,
        ThreadCreationError,
        UnhandledException,
        RelayError,

        ChannelEmpty,
        ChannelFull,
        ChannelClosed,
        ChannelTimeout
    };

    char const* to_string() const noexcept {
        switch (value) {
        case Undefined: return "Undefined";
        case Success: return "Success";
        case Pending: return "Pending";
        case InvalidChannelSize: return "InvalidChannelSize";
        case ThreadCreationError: return "ThreadCreationError";
        case UnhandledException: return "UnhandledException";
        case RelayError: return "RelayError";
        case ChannelEmpty: return "ChannelEmpty";
        case ChannelFull: return "ChannelFull";
        case ChannelClosed: return "ChannelClosed";
        case ChannelTimeout: return "ChannelTimeout";
        default: return "Invalid return code";
        }
    }

    message_framework_return_code() {
        value = 0;
    }

    message_framework_return_code(message_framework_return_code const& other) {
        value = other.value;
    }

    message_framework_return_code(int ivalue) {
        value = ivalue;
    }

    operator int() const noexcept {
        return value;
    }

    operator bool() const noexcept {
        return value == Success;
    }

    bool operator!() const noexcept {
        return value != Success;
    }

    message_framework_return_code& operator=(int const new_value) noexcept {
        value = new_value;
        return *this;
    }

    bool operator==(message_framework_return_code const& rhs) const noexcept {
        return value == rhs.value;
    }

    bool operator==(int const rhs) const noexcept {
        return value == rhs;
    }

    bool operator!=(message_framework_return_code const& rhs) const noexcept {
        return value != rhs.value;
    }

    bool operator!=(int const rhs) const noexcept {
        return value != rhs;
    }

    int value;
};

using msgfw_retcode = message_framework_return_code;

namespace transmission_policy {

using mchain_callback = void(*)(void*, message_framework_return_code);

constexpr int MChainFlag_Pending = 0x01;

// Note that mchain_node can be initialized by zero
class mchain_node {
public:
    struct _mchain_control_block: list_builder_node {
        int mchain_flags = 0;
        int mchain_reserved = 0;
        mchain_callback callback = nullptr;
    } _mchain_ctl;

    int set_mchain_flags(int const opt) noexcept {
        _mchain_ctl.mchain_flags |= opt;
        return _mchain_ctl.mchain_flags;
    }

    bool test_mchain_flags(int const opt) const noexcept {
        return _mchain_ctl.mchain_flags & opt;
    }

    void clear_mchain_properties() noexcept {
        _mchain_ctl.reset_links();
        _mchain_ctl.mchain_flags = 0;
    }

    void set_chained_callback(mchain_callback const callback) {
        _mchain_ctl.callback = callback;
    }
};

} // namespace transmission_policy
} // namespace ash

#endif // !ASH_CONCURRENCY_DETAIL_MESSAGE_PASSING_INTERNALS_H
