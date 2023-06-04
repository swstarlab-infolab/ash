//#include <ash/direct_io.h>
//#include <ash/pointer.h>
//#include <ash/size.h>
//#include <ash/detail/malloc.h>
//#include <assert.h>
//#include <string.h>
//#include <string>
//#include <sys/stat.h>
//#if defined(ASH_TOOLCHAIN_MSVC)
//#define WIN32_LEAN_AND_MEAN
//#include <Windows.h>
//#include <BaseTsd.h>
//#else
//#include <sys/types.h>
//#include <unistd.h>
//#include <fcntl.h>
//#endif
//
//namespace ash {
//
//#if defined(ASH_TOOLCHAIN_MSVC)
//#define INVALID_FILE_POINTER INVALID_HANDLE_VALUE
//int64_t pread64(HANDLE fd, void* buf, uint64_t count, uint64_t offset) {
//    assert(is_aligned_address(buf, DiskSectorSize));
//    LARGE_INTEGER offset_w32;
//    DWORD transferred;
//    assert(MAXLONGLONG >= offset);
//    assert(count % DiskSectorSize == 0);
//    assert(count <= GiB(2llu));
//    assert(offset % DiskSectorSize == 0);
//    offset_w32.QuadPart = static_cast<LONGLONG>(offset);
//    if (ASH_UNLIKELY(SetFilePointerEx(fd, offset_w32, NULL, FILE_BEGIN) == 0))
//        return -1;
//    if (ASH_UNLIKELY(ReadFile(fd, buf, static_cast<DWORD>(count), &transferred, NULL) == FALSE))
//        return -1;
//    return transferred;
//}
//#else
//#define INVALID_FILE_POINTER (-1)
//#endif
//
//struct direct_fstream::impl: noncopyable {
//    void init(uint32_t iobuf_size = MiB(32)) noexcept;
//    void cleanup() noexcept;
//    bool open(const char* filepath);
//    bool close() noexcept;
//    int64_t read(void* buf, int64_t count, uint64_t offset) const;
//    bool is_open() const noexcept;
//    char const* path() const;
//    uint64_t file_size() const;
//
//    uint32_t _iobuf_size;
//
//#if defined(ASH_TOOLCHAIN_MSVC)
//    HANDLE _fd;
//#else
//    int _fd;
//#endif
//    char* _iobuf;
//    std::string _path;
//    size_t _size;
//};
//
//void direct_fstream::impl::init(uint32_t iobuf_size) noexcept {
//    _iobuf_size = iobuf_size;
//    _fd = INVALID_FILE_POINTER;
//    _iobuf = static_cast<char*>(aligned_malloc(iobuf_size, DiskSectorSize));
//    _size = 0;
//}
//
//void direct_fstream::impl::cleanup() noexcept {
//    if (is_open())
//        close();
//    aligned_free(_iobuf);
//}
//
//bool direct_fstream::impl::open(const char* filepath) {
//    assert(is_open() == false);
//#if defined(ASH_TOOLCHAIN_MSVC)
//    DWORD const dwFlagsAndAttributes = FILE_ATTRIBUTE_READONLY;
//    _fd = CreateFileA(
//        filepath,
//        GENERIC_READ,
//        NULL,
//        // Prevents other processes from opening a file or device if they request delete, read, or write access.
//        nullptr,
//        OPEN_EXISTING,
//        dwFlagsAndAttributes,
//        nullptr);
//
//    if (_fd == INVALID_HANDLE_VALUE) {
//        return false;
//    }
//    _path = filepath;
//    _size = static_cast<uint64_t>(get_file_size(filepath));
//    return true;
//#else
//    _fd = open64(filepath, O_RDONLY | O_SYNC);
//    if (_fd < 0) {
//        return false;
//    }
//    _path = filepath;
//    _size = static_cast<uint64_t>(get_file_size(filepath));
//    return true;
//#endif
//}
//
//bool direct_fstream::impl::close() noexcept {
//    assert(is_open() == true);
//    _path.clear();
//    _size = 0;
//#if defined(ASH_TOOLCHAIN_MSVC)
//    if (_fd == INVALID_HANDLE_VALUE) {
//        return false;
//    }
//    if (CloseHandle(_fd) == FALSE) {
//        return false;
//    }
//    _fd = INVALID_HANDLE_VALUE;
//    return true;
//#else
//    if (_fd == INVALID_FILE_POINTER) {
//        return false;
//    }
//    if (0 != ::close(_fd)) {
//        return false;
//    }
//    _fd = INVALID_FILE_POINTER;
//    return true;
//#endif
//}
//
//int64_t direct_fstream::impl::read(void* buf, int64_t const count, uint64_t offset) const {
//    /* Direct I/O requires the following conditions
//     * 1. A destination buffer address must be aligned by the sector size
//     * 2. An offset of the file pointer must be aligned by the sector size
//     * 3. A requested I/O size must be multiple of the sector size
//    */
//
//    assert(is_open() == true);
//    assert(buf != nullptr);
//    assert(count > 0);
//
//    if (offset == _size)
//        return 0; // Eof
//    if (offset > _size)
//        return -1; // Invalid offset
//
//    int64_t bytes_remained = (static_cast<uint64_t>(count) < _size - offset) ? count : static_cast<int64_t>(_size - offset);
//    int64_t bytes_transferred = 0;
//
//    // Check the offset is aligned
//    if (!is_aligned_offset(offset, DiskSectorSize)) {
//        uint64_t const misaligned_size = offset % DiskSectorSize;
//        uint64_t const aligned_offset = offset - misaligned_size;
//        int64_t const pread_rslt = pread64(_fd, _iobuf, DiskSectorSize, aligned_offset);
//        if (pread_rslt == -1)
//            return -1;
//        assert(pread_rslt <= DiskSectorSize);
//        int64_t const data_size = pread_rslt - misaligned_size;
//        int64_t const copy_size = (bytes_remained < data_size) ? bytes_remained : data_size;
//        memcpy(buf, seek_pointer(_iobuf, misaligned_size), copy_size);
//        bytes_remained -= copy_size;
//        bytes_transferred += copy_size;
//        if (bytes_remained == 0)
//            return count;
//        buf = seek_pointer(buf, copy_size);
//        offset = aligned_offset + DiskSectorSize;
//    }
//
//    // Check the buffer address is aligned
//    if (is_aligned_address(buf, DiskSectorSize)) {
//        // aligned buffer
//        while (true) {
//            int64_t const pread_rslt = pread64(_fd, buf, _iobuf_size, offset);
//            if (ASH_UNLIKELY(pread_rslt == -1))
//                return -1;
//            if (ASH_UNLIKELY(pread_rslt == 0))
//                break;
//            int64_t const copy_size = (bytes_remained < pread_rslt) ? bytes_remained : pread_rslt;
//            bytes_remained -= copy_size;
//            bytes_transferred += copy_size;
//            if (bytes_remained == 0 || pread_rslt != _iobuf_size)
//                break;
//            buf = seek_pointer(buf, copy_size);
//            offset += pread_rslt;
//        }
//    }
//    else {
//        // misaligned buffer
//        while (true) {
//            int64_t const pread_rslt = pread64(_fd, _iobuf, _iobuf_size, offset);
//            if (ASH_UNLIKELY(pread_rslt == -1))
//                return -1;
//            if (ASH_UNLIKELY(pread_rslt == 0))
//                break;
//            int64_t const copy_size = (bytes_remained < pread_rslt) ? bytes_remained : pread_rslt;
//            memcpy(buf, _iobuf, copy_size);
//            bytes_remained -= copy_size;
//            bytes_transferred += copy_size;
//            if (bytes_remained == 0 || pread_rslt != _iobuf_size)
//                break;
//            buf = seek_pointer(buf, copy_size);
//            offset += pread_rslt;
//        }
//    }
//    return bytes_transferred;
//}
//
//bool direct_fstream::impl::is_open() const noexcept {
//    return _fd != INVALID_FILE_POINTER;
//}
//
//char const* direct_fstream::impl::path() const {
//    return _path.c_str();
//}
//
//uint64_t direct_fstream::impl::file_size() const {
//    return _size;
//}
//
//bool file_exists(char const* path) noexcept {
//#if defined(ASH_TOOLCHAIN_MSVC)
//    struct _stat64 buf;
//    return (__stat64(path, &buf) == 0);
//#else
//    struct stat64 buf;
//    return (stat64(path, &buf) == 0);
//#endif
//}
//
//int64_t get_file_size(char const* path) noexcept {
//#if defined(ASH_TOOLCHAIN_MSVC)
//    struct _stat64 st;
//    if (__stat64(path, &st) == -1)
//        return -1;
//    return st.st_size;
//#else
//    struct stat64 st;
//    if (stat64(path, &st) == -1)
//        return -1;
//    return st.st_size;
//#endif
//}
//
//direct_fstream::direct_fstream(uint32_t iobuf_size) {
//    _pimpl = new impl();
//    _pimpl->init(iobuf_size);
//}
//
//direct_fstream::~direct_fstream() noexcept {
//    _pimpl->cleanup();
//    delete _pimpl;
//}
//
//bool direct_fstream::open(const char* filepath) {
//    return _pimpl->open(filepath);
//}
//
//bool direct_fstream::close() noexcept {
//    return _pimpl->close();
//}
//
//int64_t direct_fstream::read(void* buf, uint64_t const count, uint64_t const offset) const {
//    return _pimpl->read(buf, count, offset);
//}
//
//bool direct_fstream::is_open() const {
//    return _pimpl->is_open();
//}
//
//char const* direct_fstream::path() const {
//    return _pimpl->path();
//}
//
//uint64_t direct_fstream::file_size() const {
//    return _pimpl->file_size();
//}
//
//} // !namespace ash
