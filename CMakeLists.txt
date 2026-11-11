cmake_minimum_required(VERSION 3.16)
project(Salap VERSION 0.4.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
# Link the C++ runtime into the program so it runs without extra installs.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
  # Sources contain Thai text; read and emit them as UTF-8.
  add_compile_options(/utf-8 /W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

add_library(salap_core STATIC
  core/src/cycle.cpp
  core/src/encoding.cpp
  core/src/helpers.cpp
  core/src/layout.cpp
  core/src/numbers.cpp
  core/src/salap_c.cpp
)
target_include_directories(salap_core PUBLIC core/include)

enable_testing()
add_executable(salap_core_tests tests/core_tests.cpp)
target_link_libraries(salap_core_tests PRIVATE salap_core)
add_test(NAME core COMMAND salap_core_tests)

if(WIN32)
  enable_language(RC)
  add_executable(Salap WIN32 windows/Salap.cpp windows/Salap.rc)
  # So the resource compiler finds Salap.ico next to Salap.rc.
  target_include_directories(Salap PRIVATE windows)
  target_link_libraries(Salap PRIVATE salap_core gdiplus comctl32 comdlg32 shell32 user32 gdi32 advapi32)
  if(MINGW)
    target_link_options(Salap PRIVATE -municode -static)
  endif()
endif()
