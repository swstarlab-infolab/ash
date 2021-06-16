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

char* replace_character(char* s, char const find, char const replace) {
    size_t const len = strlen(s);
    for (size_t i = 0; i < len; ++i) {
        if (s[i] == find)
            s[i] = replace;
    }
    return s;
}

void ltrim(std::string& s, char const c) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [c](char ch) {
        return !(ch == c);
        }));
}

void rtrim(std::string& s, char const c) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [c](char ch) {
        return !(c == ch);
        }).base(), s.end());
}

void trim(std::string& s, char c) {
    ltrim(s, c);
    rtrim(s, c);
}

std::string ltrim_copy(std::string s, char c) {
    ltrim(s, c);
    return s;
}

std::string rtrim_copy(std::string s, char c) {
    rtrim(s, c);
    return s;
}

std::string trim_copy(std::string s, char c) {
    trim(s, c);
    return s;
}

char* trim_path(char* path, char const separator) {
    char* pos = strrchr(path, separator);
    if (pos == (path + (strlen(path) - 1)))
        *pos = 0;
    return path;
}

void trim_path(std::string& path, char const separator) {
    rtrim(path, separator);
}

std::string trim_path_copy(std::string path, char const separator) {
    trim_path(path, separator);
    return path;
}

} // !namespace ash
