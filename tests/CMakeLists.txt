cmake_minimum_required(VERSION 3.16)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE TESTS_SRC "src/*.cpp")

# Add test executable
add_executable(voltrum_tests ${TESTS_SRC})

# Link with engine
target_link_libraries(voltrum_tests PRIVATE voltrum_core)
target_include_directories(voltrum_tests PUBLIC "src")

enable_testing()
add_test(voltrum_unit_tests voltrum_tests)
