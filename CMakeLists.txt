cmake_minimum_required(VERSION 3.16)
project(Salap VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  # Sources contain Thai text; read and emit them as UTF-8.
  add_compile_options(/utf-8 /W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

add_library(salap_core STATIC
  core/src/encoding.cpp
  core/src/layout.cpp
  core/src/numbers.cpp
  core/src/salap_c.cpp
)
target_include_directories(salap_core PUBLIC core/include)

enable_testing()
add_executable(salap_core_tests tests/core_tests.cpp)
target_link_libraries(salap_core_tests PRIVATE salap_core)
add_test(NAME core COMMAND salap_core_tests)
