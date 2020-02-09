cmake_minimum_required(VERSION 3.10)


# creating threads target
find_package(Threads REQUIRED)
add_library(threads SHARED IMPORTED GLOBAL)
set_target_properties(threads PROPERTIES IMPORTED_LOCATION ${CMAKE_THREAD_LIBS_INIT})


# creating alsa target
find_package(ALSA REQUIRED)
add_library(alsa SHARED IMPORTED GLOBAL)
set_target_properties(alsa PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${ALSA_INCLUDE_DIRS})
set_target_properties(alsa PROPERTIES IMPORTED_LOCATION ${ALSA_LIBRARIES})


# handling g3logger dependency
option(G3_SHARED_LIB "Build shared library" OFF)
option(USE_DYNAMIC_LOGGING_LEVELS "Turn ON/OFF log levels. An disabled level will not push logs of that level to the sink." ON)
option(ADD_FATAL_EXAMPLE "Fatal (fatal-crashes/contract) examples" OFF)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/g3log)

# TODO: review
target_include_directories(
    g3logger
    PUBLIC ${CMAKE_BINARY_DIR}/dependencies/g3log/include
)


# handling conwrap2 dependency
set(
    CONWRAP2_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/conwrap2/include
    CACHE PATH "conwrap2 include directory"
)

add_library(conwrap2 INTERFACE)
target_include_directories(
    conwrap2 INTERFACE
    $<BUILD_INTERFACE:${CONWRAP2_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${conwrap2}>
)


# TODO: implement proper package resolution
#find_package(FLAC REQUIRED)
#find_package(FLAC++ REQUIRED)


# handling type_safe dependency
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/type_safe)


# handling cxxopts dependency
option(CXXOPTS_BUILD_EXAMPLES "Set to ON to build examples" OFF)
option(CXXOPTS_BUILD_TESTS "Set to ON to build tests" OFF)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/cxxopts)


# handling networking-ts dependency
set(
    NETWORKING_INCLUDE_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/networking-ts-impl/include
    CACHE PATH "Networking TS include directory"
)

add_library(networking INTERFACE)
target_include_directories(
    networking INTERFACE
    $<BUILD_INTERFACE:${NETWORKING_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${networking}>
)


# handling scope_guard dependency
set(
    SCOPE_GUARD_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/scope_guard/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/scope_guard/include/boost"
    CACHE PATH "scope_guard include directories"
)

# for convenience setting up a target
add_library(scope_guard INTERFACE)

target_include_directories(
    scope_guard INTERFACE
    "$<BUILD_INTERFACE:${SCOPE_GUARD_INCLUDE_DIR}>"
    $<INSTALL_INTERFACE:${scope_guard}>
)


# handling any_invocable dependency
set(
    ANY_INVOCABLE_INCLUDE_DIR
    "${CMAKE_CURRENT_SOURCE_DIR}/any_invocable/include"
    CACHE PATH "any_invocable include directories"
)

# for convenience setting up a target
add_library(any_invocable INTERFACE)

target_include_directories(
    any_invocable INTERFACE
    "$<BUILD_INTERFACE:${ANY_INVOCABLE_INCLUDE_DIR}>"
    $<INSTALL_INTERFACE:${any_invocable}>
)

# handling gtest dependency
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/googletest EXCLUDE_FROM_ALL)
