cmake_minimum_required(VERSION 3.10)

# Set policy to highest version, up to 3.13
if(${CMAKE_VERSION} VERSION_LESS 3.13)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.13)
endif()


#################
# Configuration #
#################

set(LOCAL_PROJECT_VENDOR      "Andrej Kislovskij")
set(LOCAL_PROJECT_NAME        "SlimStreamer")
set(LOCAL_PROJECT_NAMESPACE   "Slim")
set(LOCAL_PROJECT_DESCRIPTION "SlimStreamer is streaming system based on SlimProto protocol")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "CMAKE_BUILD_TYPE may be one of: Release or Debug" FORCE)
endif()
message("Build Type is ${CMAKE_BUILD_TYPE}")

execute_process(
    COMMAND git describe --dirty --always --tags
    OUTPUT_VARIABLE LOCAL_PROJECT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

project(
    ${LOCAL_PROJECT_NAME}
    DESCRIPTION ${LOCAL_PROJECT_DESCRIPTION}
    LANGUAGES CXX
)

# defining C++ version globally
set(CMAKE_CXX_STANDARD          17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS        OFF)

# adding sanitizers only if CMAKE_BUILD_TYPE is DEBUG
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=leak")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=undefined")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-optimize-sibling-calls")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize-address-use-after-scope")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer")

# placing binaries and libraries according to GNU standards
include(GNUInstallDirs)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR})

add_library(
    SlimStreamerLib OBJECT
    src/slim/alsa/Source.cpp
    src/slim/log/ConsoleSink.cpp
    src/slim/log/SinkFilter.cpp
)

target_include_directories(
    SlimStreamerLib
    PUBLIC 
        $<INSTALL_INTERFACE:include>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

target_compile_definitions(
    SlimStreamerLib
    PUBLIC VERSION="${LOCAL_PROJECT_VERSION}"
    PUBLIC SCOPE_GUARD_STANDALONE
)

# TODO: enable support for Clang
#if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  # using Clang
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  # using GCC
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
  # using Intel C++
#elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  # using Visual Studio C++
#endif()

target_compile_options(
    SlimStreamerLib
    PRIVATE
        $<$<CXX_COMPILER_ID:GNU>:
            -Wall
            -Wnon-virtual-dtor
            -Woverloaded-virtual
            -Wduplicated-cond
            -Wduplicated-branches
            -Wlogical-op
            -Wrestrict
            -Wnull-dereference
            -Wdouble-promotion
            -fstrict-aliasing
            -Wformat=2
# TODO: some day...
#            -Werror
#            -pedantic-errors
#            -Wextra
#            -Wconversion
#            -Wsign-conversion
#            -Wshadow
        >
)

add_executable(
    SlimStreamer
    src/SlimStreamer.cpp
)


################
# Dependencies #
################

add_subdirectory(dependencies)

target_link_libraries(
    SlimStreamerLib
    threads
    alsa
    ${CMAKE_THREAD_LIBS_INIT}
    FLAC
    FLAC++
    g3logger
    cxxopts
    type_safe
    conwrap2
    networking
    scope_guard
)

target_link_libraries(
    SlimStreamer
    SlimStreamerLib
)

###########
# Testing #
###########

option(BUILD_TESTS "Build the tests" OFF)
if (${BUILD_TESTS})
    include(CTest)
    enable_testing()
    add_subdirectory(test)
endif()
