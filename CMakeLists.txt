cmake_minimum_required(VERSION 3.14)
project(certamen LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ftxui QUIET)

if (NOT ftxui_FOUND)
  include(FetchContent)
  FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v6.1.9
  )
  FetchContent_MakeAvailable(ftxui)
endif()

# set(FTXUI_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
# set(FTXUI_BUILD_DOCS OFF CACHE BOOL "" FORCE)

find_package(yaml-cpp REQUIRED)
find_package(libssh REQUIRED)

file(GLOB_RECURSE SOURCES src/*.cpp)
add_executable(certamen ${SOURCES})

target_include_directories(certamen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(certamen
  PRIVATE ftxui::screen ftxui::dom ftxui::component
  PRIVATE yaml-cpp
  PRIVATE ssh
  PRIVATE util
)

set_target_properties(certamen PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
