cmake_minimum_required(VERSION 3.6)
project(SmallVec)

set(CMAKE_CXX_STANDARD 20)

#======================================

include(FetchContent)

FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest
)

FetchContent_MakeAvailable(googletest)

#======================================

set(SMALLVEC_INCLUDE_DIR project/include)

#======================================

file(GLOB_RECURSE TEST_FILES project/test/*.cpp)

enable_testing()
add_executable(tests ${TEST_FILES})
target_include_directories(tests PRIVATE project/include)
target_link_libraries(tests PRIVATE gtest_main)
