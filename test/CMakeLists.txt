
# TODO: some day...
# using this to get code coverage
#if (CMAKE_CXX_COMPILER_ID MATCHES GNU)
#    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
#endif()

add_executable(
    SlimStreamerTest
    ${CMAKE_CURRENT_SOURCE_DIR}/SlimStreamerTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/proto/StreamingSessionTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/ArrayTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/BufferPoolTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/HeapBufferTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/HelperTest.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/slim/util/buffer/RingTest.cpp
)

target_compile_definitions(
    SlimStreamerTest
    PUBLIC VERSION="${LOCAL_PROJECT_VERSION}"
    PUBLIC SCOPE_GUARD_STANDALONE
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
