#include <ash/io/binary_file_stream.h>

#ifdef ASH_ENV_WINDOWS

#include <ash/detail/malloc.h>
#include <ash/pointer.h>
#include <ash/size.h>
#include <Windows.h>
#include <assert.h>

namespace ash {

class direct_binary_file_stream_win32 final : public binary_file_stream {
public:
    explicit direct_binary_file_stream_win32(configure_t const& cfg);
    ~direct_binary_file_stream_win32() noexcept override;
    bool        open(std::string path) override;
    bool        close() noexcept override;
    bool        is_open() const noexcept override;
    bool        read(void* buf, offset_t offset, size_t count) override;
    uint64_t    gcount() const noexcept override;
    char const* path() const noexcept override;
    error_code_t get_last_error() const noexcept override;
    uint64_t get_file_size() const noexcept override;

    static int64_t pread64(HANDLE fd, void* buf, uint64_t count, uint64_t offset);

private:
    configure_t  _cfg;
    std::string  _path;
    HANDLE       _fd;
    error_code_t _ec;
    uint64_t     _file_size;
    uint64_t     _last_extracted;

    static constexpr int64_t Win32SeekError = -1;
    static constexpr int64_t Win32ReadError = -2;
};

direct_binary_file_stream_win32::direct_binary_file_stream_win32(configure_t const& cfg) {
    _cfg = cfg;
    _path = "undefined";
    _fd = INVALID_HANDLE_VALUE;
    _ec = error_code_t::Undefined;
    _file_size = 0;
    _last_extracted = 0;
}

direct_binary_file_stream_win32::~direct_binary_file_stream_win32() noexcept {
    close();
}

bool direct_binary_file_stream_win32::open(std::string path) {
    assert(!is_open());
    _path = std::move(path);
    DWORD const dwFlagsAndAttributes = FILE_FLAG_NO_BUFFERING;
    _fd = CreateFileA(
        _path.data(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        dwFlagsAndAttributes,
        nullptr
    );

    if (_fd == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "CreateFileA failure; GetLastError() = %u\n", (unsigned)GetLastError());
        return false;
    }

    {
        DWORD       high32 = 0;
        DWORD const low32 = GetFileSize(_fd, &high32);
        _file_size = high32;
        _file_size <<= 32;
        _file_size |= low32;
    }

    return _fd != INVALID_HANDLE_VALUE;
}

bool direct_binary_file_stream_win32::close() noexcept {
    if (!is_open())
        return true;

    assert(_fd != INVALID_HANDLE_VALUE);

    try {
        if (CloseHandle(_fd) == FALSE)
            return false;
        _path = "undefined (closed)";
        _fd = INVALID_HANDLE_VALUE;
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "An exception occurred in function %s at line %d\ndetail: %s\n", __FUNCTION__, __LINE__, ex.what());
    }
    catch (...) {
        fprintf(stderr, "An unknown exception occurred in function %s at line %d\n", __FUNCTION__, __LINE__);
    }

    return true;
}

bool direct_binary_file_stream_win32::is_open() const noexcept {
    return _fd != INVALID_HANDLE_VALUE;
}

bool direct_binary_file_stream_win32::read(void* buf, offset_t offset, size_t count) {
    /* Direct I/O requires the following conditions
     * 1. A destination buffer address must be aligned by the sector size
     * 2. An offset of the file pointer must be aligned by the sector size
     * 3. A requested I/O size must be multiple of the sector size
    */

    assert(is_open());
    assert(buf != nullptr);
    assert(is_aligned_address(buf, _cfg.alignment));
    assert(is_aligned_offset(offset, _cfg.alignment));
    assert(is_aligned_offset(count, _cfg.alignment));

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

    int64_t const r = pread64(_fd, buf, count, offset);
    if (r < 0) {
        if (r == Win32SeekError)
            _ec = error_code_t::SeekError;
        else
            _ec = error_code_t::ReadError;
        _last_extracted = 0;
        return false;
    }
    _last_extracted = static_cast<uint64_t>(r);

    return true;
}

size_t direct_binary_file_stream_win32::gcount() const noexcept {
    return _last_extracted;
}

char const* direct_binary_file_stream_win32::path() const noexcept {
    return _path.c_str();
}

binary_file_stream::error_code_t direct_binary_file_stream_win32::get_last_error() const noexcept {
    return _ec;
}

uint64_t direct_binary_file_stream_win32::get_file_size() const noexcept {
    return _file_size;
}

int64_t direct_binary_file_stream_win32::pread64(
    HANDLE         fd,
    void* buf,
    uint64_t const count,
    uint64_t const offset) {
    assert(is_aligned_address(buf, DiskSectorSize));
    int64_t total_transferred = 0;
    assert(MAXLONGLONG >= offset);
    assert(count % DiskSectorSize == 0);
    assert(offset % DiskSectorSize == 0);

    {
        LARGE_INTEGER offset_w32;
        offset_w32.QuadPart = static_cast<LONGLONG>(offset);
        if (ASH_UNLIKELY(SetFilePointerEx(fd, offset_w32, NULL, FILE_BEGIN) == 0))
            return Win32SeekError;
    }

    uint64_t remained = count;
    while (true) {
        DWORD const read_size = (remained > GiB<DWORD>(2)) ? GiB<DWORD>(2) : static_cast<DWORD>(remained);
        DWORD transferred;
        if (ASH_UNLIKELY(ReadFile(fd, seek_pointer(buf, total_transferred), read_size, &transferred, NULL) == FALSE)) {
            fprintf(stderr, "ReadFile failure; GetLastError() = %u\n", (unsigned)GetLastError());
            return Win32ReadError;
        }
        remained -= transferred;
        total_transferred += transferred;
        if (remained == 0 || transferred < read_size)
            break;
    }

    return total_transferred;
}

binary_file_stream* make_direct_fstream(binary_file_stream::configure_t const& cfg) {
    return new direct_binary_file_stream_win32(cfg);
}

void release_direct_fstream(binary_file_stream* s) {
    delete s;
}

} // namespace ash

#endif
