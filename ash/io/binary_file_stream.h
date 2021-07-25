#ifndef ASH_IO_DIRECT_STREAM_H
#define ASH_IO_DIRECT_STREAM_H
#include <string>
#include <ash/config.h>

namespace ash {

// abstract class binary_file_stream
class binary_file_stream {
public:
    struct configure_t {
        uint32_t alignment = 0;
    };

    enum class error_code_t {
        Success = 0,
        Undefined = 1,
        InvalidArgument,
        SeekError,
        ReadError,
        PosixPread64Error,
    };

    using offset_t = uint64_t;
    virtual              ~binary_file_stream() noexcept = default;
    virtual bool         open(std::string path) = 0;
    virtual bool         close() noexcept = 0;
    virtual bool         is_open() const noexcept = 0;
    virtual bool         read(void* buf, offset_t offset, size_t size) = 0;
    virtual uint64_t     gcount() const noexcept = 0;
    virtual char const* path() const noexcept = 0;
    virtual error_code_t get_last_error() const noexcept = 0;
    virtual uint64_t     get_file_size() const noexcept = 0;
};

#if defined(ASH_ENV_WINDOWS)
class direct_binary_file_stream_win32;
binary_file_stream* make_direct_fstream(binary_file_stream::configure_t const& cfg);
#elif defined(ASH_ENV_UNIX)
class direct_binary_file_stream_posix;
binary_file_stream* make_direct_fstream(binary_file_stream::configure_t const& cfg);
#endif

void release_direct_fstream(binary_file_stream* s);

class binary_file_stream_cxxstd;
binary_file_stream* make_fstream();

} // namespace ash

#endif // ASH_IO_DIRECT_STREAM_H
