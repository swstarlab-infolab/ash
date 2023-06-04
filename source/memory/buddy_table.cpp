#include <ash/memory/buddy_table.h>
#include <ash/numeric.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits>
#include <iterator>
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

namespace ash {
namespace buddy_impl {

struct buddy_table_size_info {
    level_t tbl_size;
    level_t buddy_lv;
};

/*
 * Root: 200
 * Minimum coefficient: 3
 *
 * Coefficient tree
 *              +---          [232]             ---> 232 [Root]
 *              |               |
 *              |             [116]             ---> 116
 *   Linear  <--+               |
 *              |             [58]              ---> 58
 *              |               |
 *              +---          [29]              ---> 29
 *              |            /    \
 *              |         [15]     [14]         ---> 15, 14
 *   Binary  <--+        /  \       /  \
 *              |      [8]  [7]    [7]  [7]     ---> 8, 7 [A1B3-Pattern]
 *              |      / \  / \    / \   / \
 *              +--- [4][4][4][3] [4][3][4][3]  ---> 4, 3 [A3B1-Pattern]
 *
 * Buddy table
 * - Flag: Unique (U), Frequent (F), Rare (R), A1B3-Pattern, A3B1-Pattern
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * | Idx | Lev |  Cof  | U | F | R | Pattern | Dist | Off |
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * |  0  |  0  |  232  | # |   |   |   N/A   |   0  |  0  |
 * |  1  |  1  |  116  | # |   |   |   N/A   |   1  |  0  |
 * |  2  |  2  |   58  | # |   |   |   N/A   |   1  |  0  |
 * |  3  |  3  |   29  | # |   |   |   N/A   |   1  |  0  |
 * |  4  |  4  |   15  |   |   | # |   A3B1* |   1  |  0  |
 * |  5  |  4  |   14  |   |   | # |   A3B1* |   2  |  1  |
 * |  6  |  5  |    8  |   |   | # |   A1B3  |   2  |  0  |
 * |  7  |  5  |    7  |   | # |   |   A1B3  |   3  |  1  |
 * |  8  |  6  |    4  |   | # |   |   A3B1  |   2  |  0  |
 * |  9  |  6  |    3  |   |   | # |   A3B1  |   3  |  1  |
 * +-----+-----+-------+---+---+---+---------+------+-----+
 * Note that the patterns of a first binary level (4) are assumed to be R-A3B1*
 *
 */

buddy_table_size_info get_buddy_table_size_info(cof_type const root, cof_type const min_cof) {
    // Note that a size of root node is root * alignment
    auto const get_linear_bound = [](cof_type n, cof_type min_cof) -> cof_type {
        do {
            if (n & 0x1u || n <= min_cof)
                return n;
            n /= 2;
        } while (true);
    };

    auto const get_binary_depth = [](cof_type odd, cof_type min_cof) -> unsigned {
        assert(odd & 0x1u);
        unsigned depth = 0;
        do {
            cof_type const q = odd / 2; // quotient
            // select a next odd number
            q & 0x1u ? odd = q : odd = q + 1;
            if (q < min_cof)
                break;
            depth += 1;
        } while (true);
        return depth;
    };

    buddy_table_size_info info;
    cof_type const linear_bound = get_linear_bound(root, min_cof);
    unsigned const linear_depth = static_cast<unsigned>(log2u(static_cast<uint64_t>(root / linear_bound))) + 1;
    if (linear_bound <= min_cof) {
        info.tbl_size = info.buddy_lv = linear_depth;
    }
    else {
        unsigned const binary_depth = get_binary_depth(linear_bound, min_cof);
        info.tbl_size = linear_depth + binary_depth * 2;
        info.buddy_lv = linear_depth + binary_depth;
    }
    return info;
}

buddy_table::buddy_table() {
    _align = 0;
    _min_cof = 0;
    _tbl_size = 0;
    _buddy_lv = 0;
    memset(&_properties, 0, sizeof _properties);
}

buddy_table::~buddy_table() noexcept {
    _free_properties(&_properties);
}

buddy_table::buddy_table(cof_type const root_cof, unsigned const align, cof_type const min_cof) :
    buddy_table() {
    init(root_cof, align, min_cof);
}

void buddy_table::init(cof_type const root_cof, unsigned const align, cof_type const min_cof) {
    assert(align % 2 == 0);
    assert(min_cof > 0);
    auto const info = get_buddy_table_size_info(root_cof, min_cof);
    _align = align;
    _min_cof = min_cof;
    _tbl_size = info.tbl_size;
    _buddy_lv = info.buddy_lv;
    _properties = _init_properties(_tbl_size, root_cof, align, min_cof);
    //TODO: Add exception handling when initialization fails (11/05/2019)
}

void buddy_table::clear() noexcept {
    _free_properties(&_properties);
    _align = 0;
    _min_cof = 0;
    _tbl_size = 0;
    _buddy_lv = 0;
    memset(&_properties, 0, sizeof _properties);
}

void buddy_table::printout() const {
    for (level_t i = 0; i < _tbl_size; ++i) {
        printf("%4u | %4u | %" PRId64 "\n",
            i, _properties.level_v[i], _properties.cof_v[i]
        );
    }
}

blkidx_t buddy_table::best_fit(uint64_t const block_size) const {
    assert(block_size <= static_cast<uint64_t>(_properties.cof_v[0] * _align));
    assert(_tbl_size > 0);
    cof_type const find_cof = static_cast<cof_type>((block_size + _align - 1) / _align);

#if 1
    // Reverse lower_bound
    cof_type const* it;
    unsigned count, step;
    count = _tbl_size;

    cof_type const* first = &_properties.cof_v[_tbl_size - 1];

    while (count > 0) {
        it = first;
        step = count / 2;
        it -= step;
        if (*it < find_cof) {
            first = --it;
            count -= step + 1;
        }
        else
            count = step;
    }
    assert(block_size <= static_cast<uint64_t>(*first * _align));
    return static_cast<blkidx_t>(std::distance(const_cast<cof_type const*>(_properties.cof_v), first));
#endif

#if 0
    // Linear search v2
    for (blkidx_t i = _tbl_size - 1; i > 0; --i) {
        if (_properties.cof_v[i] >= find_cof) {
            assert(block_size <= static_cast<uint64_t>(_properties.cof_v[i] * _align));
            return i;
        }
    }
    return 0;
#endif

#if 0
    cof_type diff1 = std::numeric_limits<cof_type>::max();
    blkidx_t i = _tbl_size;
    do {
        cof_type const diff2 = std::abs(_prop.cof_v[i - 1] - cof);
        if (diff2 > diff1)
            return i;
        diff1 = diff2;
        i -= 1;
    } while (i > 0);
    return i;
#endif
}

void buddy_table::_free_properties(tbl_properties* attr) {
    free(attr->level_v);
    free(attr->cof_v);
    free(attr->prof_v);
    memset(attr, 0, sizeof(tbl_properties));
}

buddy_table::tbl_properties buddy_table::_init_properties(
    unsigned const tbl_size,
    cof_type const root,
    [[maybe_unused]] unsigned const align,
    [[maybe_unused]] cof_type const min_cof) {
    assert(align % 2 == 0);
    assert(min_cof > 0);
    assert(root > 0);

    cof_type n = root;
    tbl_properties prop;
    memset(&prop, 0, sizeof prop);
    prop.level_v = static_cast<level_t*>(calloc(tbl_size, sizeof(level_t)));
    if (prop.level_v == nullptr)
        goto lb_err;
    prop.cof_v = static_cast<cof_type*>(calloc(tbl_size, sizeof(cof_type)));
    if (prop.cof_v == nullptr)
        goto lb_err;
    prop.prof_v = static_cast<blk_prop_t*>(calloc(tbl_size, sizeof(uint8_t)));
    if (prop.prof_v == nullptr)
        goto lb_err;

    // root
    prop.level_v[0] = 0;
    prop.cof_v[0] = n;
    prop.prof_v[0].flags |= UniqueBuddyBlock;
    prop.prof_v[0].dist = 0;
    prop.prof_v[0].offset = 0;

    do {
        // linear
        blkidx_t i = 1;
        while (i < tbl_size && !(n & 0x1u)) {
            prop.level_v[i] = i;
            prop.cof_v[i] = n / 2;
            prop.prof_v[i].flags |= UniqueBuddyBlock;
            prop.prof_v[i].dist = 1;
            prop.prof_v[i].offset = 0;
            n /= 2;
            i += 1;
        }

        if (i >= tbl_size)
            break;

        assert(((tbl_size - i) & 0x1u) == 0);
        // binary
        level_t lv = i;
        blkidx_t const linear_size = i;
        bool a1b3_pattern = false;
        while (i + 1u < tbl_size) {
            assert(n & 0x1u);
            cof_type const r = n / 2;
            cof_type const l = r + 1;
            // left-side child
            prop.level_v[i] = lv;
            prop.cof_v[i] = l;
            // right-side child
            prop.level_v[i + 1] = lv;
            prop.cof_v[i + 1] = r;
            // spanning info
            if (a1b3_pattern) {
                prop.prof_v[i].flags = RareBuddyBlock | A1B3Pattern;
                prop.prof_v[i + 1].flags = FrequentBuddyBlock | A1B3Pattern;
            }
            else {
                prop.prof_v[i].flags = FrequentBuddyBlock | A3B1Pattern;
                prop.prof_v[i + 1].flags = RareBuddyBlock | A3B1Pattern;
            }
            prop.prof_v[i].dist = 2;
            prop.prof_v[i].offset = 0;
            prop.prof_v[i + 1].dist = 3;
            prop.prof_v[i + 1].offset = 1;
            // update states for next iteration
            a1b3_pattern = l & 0x1u;
            a1b3_pattern ? n = l : n = r;
            i += 2;
            lv += 1;
        }
        // fix for a first binary level
        assert(linear_size + 1 < tbl_size);
        prop.prof_v[linear_size].dist = 1;
        prop.prof_v[linear_size + 1].dist = 2;
        prop.prof_v[linear_size].flags =
            prop.prof_v[linear_size + 1].flags = RareBuddyBlock | A3B1Pattern;
    } while (false);
    goto lb_return;

lb_err:
    _free_properties(&prop);

lb_return:
    return prop;
}

} // !namespace _buddy_impl
} // !namespace ash
