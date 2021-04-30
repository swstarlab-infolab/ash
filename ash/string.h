#ifndef ASH_STRING_H
#define ASH_STRING_H
#include <ash/config.h>
#include <string>
#include <stdint.h>
#include <string.h>

namespace ash {

enum class size_prefix : int {
    EiB = 0,
    PiB,
    TiB,
    GiB,
    MiB,
    KiB,
    B
};

char* size2str_decimal(uint64_t size, char* out, size_t bufsize);
char* size2str_binary(uint64_t size, char* out, size_t bufsize);
char* size2str_binary(uint64_t size, size_prefix base, char* out, size_t bufsize);

char* dirname(char* ASH_RESTRICT buffer, size_t bufsize, char const* ASH_RESTRICT input, char separator);
void dirname(std::string& s, char separator);
std::string dirname_copy(std::string const& s, char separator);
char const* basename(char const* path, char separator);
char* replace_character(char* s, char find, char replace);

void ltrim(std::string& s, char c = 0x20);
void rtrim(std::string& s, char c = 0x20);
void trim(std::string& s, char c = 0x20);
std::string ltrim_copy(std::string s, char c = 0x20);
std::string rtrim_copy(std::string s, char c = 0x20);
std::string trim_copy(std::string s, char c = 0x20);

char* trim_path(char* path, char separator);
void trim_path(std::string& path, char separator);
std::string trim_path_copy(std::string path, char separator);

inline char* tolower_str(char* str) {
    size_t const len = strlen(str);
    for (size_t i = 0; i < len; ++i)
        str[i] = static_cast<char>(tolower(static_cast<unsigned char>(str[i])));
    return str;
}

} // !namespace ash
#endif // ASH_STRING_H
