#include <ash/memory/buddy_table.h>

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
} // !namespace _buddy_impl
} // !namespace ash
