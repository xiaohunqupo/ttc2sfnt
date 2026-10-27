cmake_minimum_required(VERSION 3.12)

# The library version is maintained by hand in inc/ttc2sfnt/ttc2sfnt.h; the build system only
# reads it and never configures it into a generated header.
project(ttc2sfnt
	DESCRIPTION "Split TrueType Collections (.ttc) into standalone fonts"
	LANGUAGES CXX)

# ----------------------------------------------------------------------------------------- options

# ON  : the headers include the .cpp files and everything is inline (no library to build).
# OFF : src/*.cpp are compiled into a static library.
option(TTC2SFNT_HEAD_ONLY "Build the library header only" ON)
option(TTC2SFNT_BUILD_TESTS "Build the regression tests" ON)
option(TTC2SFNT_BUILD_SAMPLES "Build the sample programs" ON)

# --------------------------------------------------------------------------------- configuration

# inc/ttc2sfnt/config.h is not stored: it is configured from config.h.in so the switch above ends
# up inside the header every source and consumer includes. It carries the build mode only - the
# version macros live in ttc2sfnt.h.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/inc/ttc2sfnt/config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/ttc2sfnt/config.h"
    @ONLY)

# ----------------------------------------------------------------------------------------- library

set(TTC2SFNT_SOURCES
    src/ttc2sfnt/ttc_error.cpp
    src/ttc2sfnt/ttc_file.cpp
    src/ttc2sfnt/ttc_sfnt.cpp
    src/ttc2sfnt/ttc_name.cpp
    src/ttc2sfnt/ttc_collection.cpp
    src/ttc2sfnt/ttc_extract.cpp)

if (TTC2SFNT_HEAD_ONLY)
    # Nothing to compile: consumers need the generated header, inc, and src because the headers
    # include the .cpp files.
    add_library(ttc2sfnt INTERFACE)
    target_include_directories(ttc2sfnt INTERFACE
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
else()
    add_library(ttc2sfnt STATIC ${TTC2SFNT_SOURCES})
    target_include_directories(ttc2sfnt
        PUBLIC
            "${CMAKE_CURRENT_BINARY_DIR}/generated"
            "${CMAKE_CURRENT_SOURCE_DIR}/inc"
        PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/src")   # only the sources include ttc2sfnt/common.h
    if (MSVC)
        target_compile_options(ttc2sfnt PRIVATE /W4)
    else()
        target_compile_options(ttc2sfnt PRIVATE -Wall -Wextra)
    endif()
endif()
add_library(ttc2sfnt::ttc2sfnt ALIAS ttc2sfnt)
target_compile_features(ttc2sfnt INTERFACE cxx_std_11)

message(STATUS "ttc2sfnt: header only build = ${TTC2SFNT_HEAD_ONLY}")

# Warnings for the executables, which in header only mode also compile the library.
function(ttc2sfnt_executable target)
    target_link_libraries(${target} PRIVATE ttc2sfnt)
    if (MSVC)
        target_compile_options(${target} PRIVATE /W4)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra)
    endif()
endfunction()

# ------------------------------------------------------------------------------------------- tests

if (TTC2SFNT_BUILD_TESTS)
    enable_testing()

    add_executable(ttc_tests test/test_ttc.cpp)
    ttc2sfnt_executable(ttc_tests)

    # A .ttc in fonts/ becomes part of the corpus; the test also builds its own collections, so it
    # runs with and without one.
    file(GLOB TTC_TOOL_FONT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/fonts/*.ttc")
    add_test(NAME ttc_tests COMMAND ttc_tests ${TTC_TOOL_FONT_FILES}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
endif()

# ----------------------------------------------------------------------------------------- samples

if (TTC2SFNT_BUILD_SAMPLES)
    # ttcsplit: write every face of a collection as <family>.ttf / .otf.
    add_executable(ttcsplit sample/ttcsplit.cpp)
    ttc2sfnt_executable(ttcsplit)

    # ttcdump: print the header, the table directories and the checksum state of every face.
    add_executable(ttcdump sample/ttcdump.cpp)
    ttc2sfnt_executable(ttcdump)
endif()
