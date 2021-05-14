#include <ash/string.h>
#include <algorithm>

namespace ash {

char* dirname(char* ASH_RESTRICT buffer, size_t const bufsize, char const* const ASH_RESTRICT input, char const separator) {
    // init an output buffer
    memset(buffer, 0, bufsize);

    // handle the null string
    if (input == nullptr || input[0] == '\0') {
        buffer[0] = '.';
        return buffer;
    }

    char const* p = input + strlen(input) - 1;
    // trail a last separator
    while (p > input && *p == separator)
        p -= 1;

    // trail a begin of the directory
    while (p > input && *p != separator)
        p -= 1;

    // handle the input is a single separator such as "/" or there ara no slashes
    if (p == input) {
        buffer[0] = *p == separator ? separator : '.';
        return buffer;
    }

    // trail the next separator from back
    do {
        p -= 1;
    } while (p > input && *p == separator);

    size_t const len = p - input + 1;
    if (len >= bufsize) {
        // buffer is too small
        return nullptr;
    }
    memcpy(buffer, input, len);

    return buffer;
}

void dirname(std::string& s, char separator) {
    char* buffer = static_cast<char*>(calloc(s.length() + 1, 1));
    s = dirname(buffer, s.length() + 1, s.c_str(), separator);
    free(buffer);
}

std::string dirname_copy(std::string const& s, char separator) {
    std::string s2(s);
    dirname(s2, separator);
    return s2;
}

char const* basename(char const* path, char const separator) {
    char const* s = strrchr(path, separator);
    return (s == nullptr) ? path : ++s;
}

} // !namespace ash
