cmake_minimum_required(VERSION 3.10)
project(YadroPCClub LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCLUDE_DIR include)
set(TEST_DIR    test)

file(GLOB SOLUTION_SOURCES src/*.cpp)

add_library(YadroCore
    ${SOLUTION_SOURCES}
)
target_include_directories(YadroCore
    PUBLIC ${INCLUDE_DIR}
)

add_executable(pc_club
    app/main.cpp
)
target_link_libraries(pc_club
    PRIVATE YadroCore
)
target_include_directories(pc_club
    PRIVATE ${INCLUDE_DIR}
)

include(FetchContent)

message(STATUS "Fetching Catch2...")
FetchContent_Declare(
  Catch2
  URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.0.tar.gz
  FIND_PACKAGE_ARGS
)
FetchContent_MakeAvailable(Catch2)
file(GLOB TEST_SRCS
    ${TEST_DIR}/*.cpp
)

add_executable(tests
    ${TEST_SRCS}
)
target_link_libraries(tests
    PRIVATE
      Catch2::Catch2WithMain
      YadroCore
)
target_include_directories(tests
    PRIVATE ${INCLUDE_DIR} ${TEST_DIR}
)

include(CTest)
include(Catch)
catch_discover_tests(tests)
