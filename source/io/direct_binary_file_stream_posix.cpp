#include <ash/io/binary_file_stream.h>

#ifdef ASH_ENV_UNIX

#include <ash/detail/malloc.h>
#include <ash/pointer.h>
#include <ash/size.h>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include <boost/filesystem.hpp>

namespace ash {

class direct_binary_file_stream_posix final : public binary_file_stream {
public:
    direct_binary_file_stream_posix(configure_t const& cfg);
    ~direct_binary_file_stream_posix() noexcept override;
    bool        open(std::string path) override;
    bool        close() noexcept override;
    bool        is_open() const noexcept override;
    bool        read(void* buf, offset_t offset, size_t count) override;
    uint64_t    gcount() const noexcept override;
    char const* path() const noexcept override;
    error_code_t get_last_error() const noexcept override;
    uint64_t     get_file_size() const noexcept;

private:
    configure_t  _cfg;
    std::string  _path;
    int       _fd;
    error_code_t _ec;
    uint64_t     _file_size;
    uint64_t     _last_extracted;
};

direct_binary_file_stream_posix::direct_binary_file_stream_posix(configure_t const& cfg) {
    _cfg = cfg;
    _path = "undefined";
    _fd = -1;
    _ec = error_code_t::Undefined;
    _file_size = 0;
    _last_extracted = 0;
}

direct_binary_file_stream_posix::~direct_binary_file_stream_posix() noexcept {
    close();
}

bool direct_binary_file_stream_posix::open(std::string path) {
    assert(!is_open());
    _path = std::move(path);
    _fd = open64(_path.c_str(), O_RDONLY | O_SYNC | O_DIRECT);
    _file_size = ::boost::filesystem::file_size(_path);

    if (_fd < 0)
        return false;
    return true;
}

bool direct_binary_file_stream_posix::close() noexcept {
    if (!is_open())
        return true;

    assert(_fd >= 0);

    try {
        if (::close(_fd) != 0)
            return false;
        _path = "undefined (closed)";
        _fd = -1;
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "An exception occurred in function %s at line %d\ndetail: %s\n", __FUNCTION__, __LINE__, ex.what());
    }
    catch (...) {
        fprintf(stderr, "An unknown exception occurred in function %s at line %d\n", __FUNCTION__, __LINE__);
    }

    return true;
}

bool direct_binary_file_stream_posix::is_open() const noexcept {
    return _fd >= 0;
}

bool direct_binary_file_stream_posix::read(void* buf, offset_t offset, size_t count) {
    /* Direct I/O requires the following conditions
     * 1. A destination buffer address must be aligned by the sector size
     * 2. An offset of the file pointer must be aligned by the sector size
     * 3. A requested I/O size must be multiple of the sector size
    */

    assert(is_open());
    assert(buf != nullptr);

    if (!is_aligned_address(buf, _cfg.alignment)) {
        _ec = error_code_t::InvalidArgument;
        return false;
    }

    if (!is_aligned_offset(offset, _cfg.alignment)) {
        _ec = error_code_t::InvalidArgument;
        return false;
    }

    if (count == 0 || offset == _file_size) {
        _last_extracted = 0;
        return true;
    }

    int64_t total_transferred = 0;
    uint64_t remained = count;
    while (true) {
        unsigned const read_size = (remained > GiB(1)) ? GiB(1) : static_cast<unsigned>(remained);
        int64_t transferred = pread64(_fd, seek_pointer(buf, total_transferred), read_size, offset);
        if (transferred < 0) {
            fprintf(stderr, "pread64 failure!\n");
            _ec = error_code_t::PosixPread64Error;
            return false;
        }

        remained -= transferred;
        total_transferred += transferred;
        offset += transferred;
        if (remained == 0 || transferred < (int64_t)read_size)
            break;
    }
    _last_extracted = (uint64_t)total_transferred;

    return true;
}

size_t direct_binary_file_stream_posix::gcount() const noexcept {
    return _last_extracted;
}

char const* direct_binary_file_stream_posix::path() const noexcept {
    return _path.c_str();
}

binary_file_stream::error_code_t direct_binary_file_stream_posix::get_last_error() const noexcept {
    return _ec;
}

uint64_t direct_binary_file_stream_posix::get_file_size() const noexcept {
    return _file_size;
}

binary_file_stream* make_direct_fstream(binary_file_stream::configure_t const& cfg) {
    return new direct_binary_file_stream_posix(cfg);
}

void release_direct_fstream(binary_file_stream* s) {
    delete s;
}

} // namespace ash

#endif
