#ifndef _ASH_MEMORY_BUDDY_TABLE_H_
#define _ASH_MEMORY_BUDDY_TABLE_H_
#include <ash/detail/noncopyable.h>
#include <stdint.h>

namespace ash {
namespace buddy_impl {

constexpr unsigned char UniqueBuddyBlock = 0x1u;
constexpr unsigned char FrequentBuddyBlock = 0x1u << 1;
constexpr unsigned char RareBuddyBlock = 0x1u << 2;
constexpr unsigned char A1B3Pattern = 0x1u << 3;
constexpr unsigned char A3B1Pattern = 0x1u << 4;
using cof_type = int64_t;
using blkidx_t = unsigned;
using level_t = unsigned;
typedef struct buddy_block_propoerty {
    unsigned char flags : 5;
    unsigned char dist : 2;
    unsigned char offset : 1;

    bool check(unsigned char bits) const {
        return (flags & bits) == bits;
    }
} blk_prop_t;
static_assert(sizeof(buddy_block_propoerty) == 1, "a size of buddy_block_property is not 1");

class buddy_table : noncopyable {
public:

    buddy_table();
    ~buddy_table() noexcept;
    buddy_table(cof_type root_cof, unsigned align, cof_type min_cof);
    void init(cof_type root_cof, unsigned align, cof_type min_cof);
    void clear() noexcept;
    void printout() const;

    blkidx_t best_fit(uint64_t block_size) const;

    unsigned align() const {
        return _align;
    }

    unsigned size() const {
        return _tbl_size;
    }

    level_t max_level() const {
        return _attr.level_v[_tbl_size - 1];
    }

    level_t level(blkidx_t const bidx) const {
        return _attr.level_v[bidx];
    }

    cof_type cof(blkidx_t const bidx) const {
        return _attr.cof_v[bidx];
    }

    blk_prop_t const& property(blkidx_t const bidx) const {
        return _attr.prof_v[bidx];
    }

protected:

    struct tbl_attr {
        level_t* level_v;
        cof_type* cof_v;
        blk_prop_t* prof_v;
    };
    static void _free_attr(tbl_attr* attr);
    static tbl_attr _init_attr(unsigned tbl_size, cof_type root, unsigned align, cof_type min_cof);

    unsigned _align;
    cof_type _min_cof;
    level_t  _tbl_size;
    level_t  _buddy_lv;
    tbl_attr _attr;
};

} // !namespace _buddy_impl
} // !namespace ash

#endif // !_ASH_MEMORY_BUDDY_TABLE_H_
