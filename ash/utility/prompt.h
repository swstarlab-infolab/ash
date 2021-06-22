#ifndef ASH_UTILITY_PROMPT_H
#define ASH_UTILITY_PROMPT_H
#include <string>

namespace ash {

struct yesno_prompt {
    std::string text;
    bool yes = false;
    bool no = false;

    bool operator()();
};

} // namespace ash
#endif // ASH_UTILITY_PROMPT_H
