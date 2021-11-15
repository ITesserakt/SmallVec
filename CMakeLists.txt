cmake_minimum_required(VERSION 3.6)
project(SmallVec)

set(CMAKE_CXX_STANDARD 20)

#======================================

if (${BUILD_TESTS})
  include(FetchContent)

  FetchContent_Declare(
          googletest
          GIT_REPOSITORY https://github.com/google/googletest
  )

  FetchContent_MakeAvailable(googletest)
endif ()

#======================================

add_library(smallvec SHARED project/src/SmallVec.cpp)
target_include_directories(smallvec PUBLIC project/include)

install(DIRECTORY project/include
        DESTINATION include
        COMPONENT Devel)

#======================================

if (${BUILD_TESTS})
  file(GLOB_RECURSE TEST_FILES project/test/*.cpp)

  enable_testing()
  add_executable(tests ${TEST_FILES})
  target_include_directories(tests PRIVATE project/include)
  target_link_libraries(tests PRIVATE gtest_main)
endif ()