
# TODO: some day...
# using this to get code coverage
#if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
#endif()

add_executable(
    SlimStreamerTest
    ${CMAKE_CURRENT_SOURCE_DIR}/SlimStreamerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/RingBufferTest.cpp
)

set_target_properties(
    SlimStreamerTest
    PROPERTIES
        FOLDER test
)

target_include_directories(
    SlimStreamerTest
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(
    SlimStreamerTest
    SlimStreamerLib
    gtest
    gmock
)

add_test(
    NAME
        SlimStreamerTest
    COMMAND
        ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/SlimStreamerTest
)
