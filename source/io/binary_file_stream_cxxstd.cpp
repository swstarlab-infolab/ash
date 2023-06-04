#include <ash/io/binary_file_stream.h>

#include <ash/pointer.h>
#include <ash/numeric.h>
#include <assert.h>

#include <fstream>
#include <boost/filesystem.hpp>

namespace ash {

class binary_file_stream_cxxstd final : public binary_file_stream {
public:
    binary_file_stream_cxxstd();
    ~binary_file_stream_cxxstd() noexcept override;
    bool        open(std::string path) override;
    bool        close() noexcept override;
    bool        is_open() const noexcept override;
    bool        read(void* buf, offset_t offset, size_t count) override;
    uint64_t    gcount() const noexcept override;
    char const* path() const noexcept override;
    error_code_t get_last_error() const noexcept override;
    uint64_t get_file_size() const noexcept override;

private:
    configure_t  _cfg;
    std::string  _path;
    std::fstream _fs;
    error_code_t _ec;
    uint64_t     _file_size;
    uint64_t     _last_extracted;
};

binary_file_stream_cxxstd::binary_file_stream_cxxstd() {
    _path = "undefined";
    _ec = error_code_t::Undefined;
    _file_size = 0;
    _last_extracted = 0;
}

binary_file_stream_cxxstd::~binary_file_stream_cxxstd() noexcept {
    close();
}

bool binary_file_stream_cxxstd::open(std::string path) {
    assert(!is_open());
    _path = std::move(path);
    _fs.open(_path, std::ios::in | std::ios::binary);
    _file_size = ::boost::filesystem::file_size(_path);

    return _fs.is_open();
}

bool binary_file_stream_cxxstd::close() noexcept {
    if (!is_open())
        return true;

    try {
        _fs.close();
        _path = "undefined (closed)";
    }
    catch (std::exception const& ex) {
        fprintf(stderr, "An exception occurred in function %s at line %d\ndetail: %s\n", __FUNCTION__, __LINE__, ex.what());
    }
    catch (...) {
        fprintf(stderr, "An unknown exception occurred in function %s at line %d\n", __FUNCTION__, __LINE__);
    }

    return true;
}

bool binary_file_stream_cxxstd::is_open() const noexcept {
    return _fs.is_open();
}

bool binary_file_stream_cxxstd::read(void* buf, offset_t offset, size_t count) {
    assert(is_open());
    assert(buf != nullptr);

    if (count == 0 || offset == _file_size) {
        _last_extracted = 0;
        return true;
    }

    _fs.seekg(offset, std::ios::beg);
    if (!_fs.good()) {
        _ec = error_code_t::SeekError;
        _last_extracted = 0;
        return false;
    }
    _fs.read(static_cast<char*>(buf), count);
    if (_fs.bad()) {
        _ec = error_code_t::ReadError;
        _last_extracted = 0;
        return false;
    }
    _last_extracted = static_cast<uint64_t>(_fs.gcount());

    return true;
}

size_t binary_file_stream_cxxstd::gcount() const noexcept {
    return _last_extracted;
}

char const* binary_file_stream_cxxstd::path() const noexcept {
    return _path.c_str();
}

binary_file_stream::error_code_t binary_file_stream_cxxstd::get_last_error() const noexcept {
    return _ec;
}

uint64_t binary_file_stream_cxxstd::get_file_size() const noexcept {
    return _file_size;
}

binary_file_stream* make_fstream() {
    return new binary_file_stream_cxxstd();
}

} // namespace ash

