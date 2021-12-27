#include "testdef.h"
#include <ash/io/binary_file_stream.h>
#include <ash/detail/malloc.h>
#include <ash/size.h>
#include <boost/filesystem.hpp>

using namespace ash;

TEST_CASE("I/O Perf", "[benchmark]") {
    char const* fpath = "/mnt/nvme/vee/GridFormat_v8-3GiB/RMAT31/RMAT31.grid";
    //char const* fpath = "I:\\temp\\RMAT27.grid";
    constexpr unsigned alignment     = 4096;
    uint64_t const     fsize         = boost::filesystem::file_size(fpath);
    uint64_t const     fsize_aligned = aligned_size(fsize, alignment);

    char* buf1 = static_cast<char*>(malloc(fsize));
    char* buf2 = static_cast<char*>(aligned_malloc(fsize_aligned, alignment));
    REQUIRE(buf1 != nullptr);
    REQUIRE(buf2 != nullptr);
    memset(buf1, 0, fsize);
    memset(buf2, 0, fsize_aligned);

    BENCHMARK("Standard fstream") {
        binary_file_stream* fs = make_fstream();
        REQUIRE(fs->open(fpath));
        REQUIRE(fs->read(buf1, 0, fsize));
        REQUIRE(fs->gcount() == fsize);
    }

    BENCHMARK("Direct fstream") {
        binary_file_stream::configure_t cfg;
        cfg.alignment = alignment;
        binary_file_stream* fs = make_direct_fstream(cfg);
        REQUIRE(fs->open(fpath));
        REQUIRE(fs->read(buf2, 0, fsize_aligned));
        REQUIRE(fs->gcount() == fsize);
    }

    REQUIRE(memcmp(buf1, buf2, fsize) == 0);

    free(buf1);
    aligned_free(buf2);
}
