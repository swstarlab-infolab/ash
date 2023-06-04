#include <ash/utility/prompt.h>
#include <iostream>

namespace ash {

bool yesno_prompt::operator()() {
    std::cout << text << std::endl;
    char c;
    do {
        std::cout << "prompt (y/n)> ";
        std::cin >> c;
    } while (!std::cin.fail() && c != 'y' && c != 'n');
    if (c == 'y') {
        yes = true;
        no  = false;
    }
    else {
        yes = false;
        no  = true;
    }
    return yes;
}
} // namespace ash