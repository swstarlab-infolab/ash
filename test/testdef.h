#pragma once
#define CATCH_CONFIG_ENABLE_BENCHMARKING
#include <ash/config.h>
#include <catch2/catch.hpp>
#include <string>

std::string _get_test_data_dir(char const* code_path, char separator = ASH_PATH_SEPARATOR);
std::string _get_test_data_path(char const* code_path, char const* test_file_name, char separator = ASH_PATH_SEPARATOR);

#define MAKE_TEST_DATA_DIR() _get_test_data_dir(__FILE__)
#define MAKE_TEST_DATA_PATH(TestFile) _get_test_data_path(__FILE__, TestFile)

