#include "testdef.h"
#include <ash/string.h>

std::string _get_test_data_dir(char const* code_path, char separator) {
    std::string path = ash::dirname_copy(std::string{ code_path }, separator);
    return path;
}

std::string _get_test_data_path(char const* code_path, char const* test_file_name, char separator) {
    std::string path = _get_test_data_dir(code_path, separator);
    path += separator;
    path += test_file_name;
    return path;
}
