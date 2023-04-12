cmake_minimum_required(VERSION 3.14)
project(mantissa LANGUAGES CXX)

add_library(
  mantissa
  src/mantissa.cpp
)
target_include_directories(
  mantissa
  PUBLIC
  src
)

add_executable(
  mantissa_dev
  src/main.cpp
)
target_link_libraries(
  mantissa_dev
  PUBLIC
  mantissa
)

include(CTest)
if (BUILD_TESTING)
  file(GLOB PASSING_TESTS tst/pass_*.cpp)
  foreach(test ${PASSING_TESTS})
    string(RANDOM testname)
    add_executable(
      ${testname}
      ${test}
    )
    target_include_directories(
      ${testname}
      PUBLIC
      src/
    )
    add_test(
      NAME ${test}
      COMMAND $<TARGET_FILE:${testname}>
    )
  endforeach()
  file(GLOB FAILING_TESTS tst/fail_*.cpp)
  foreach(test ${FAILING_TESTS})
    string(RANDOM testname)
    add_executable(
      ${testname}
      ${test}
    )
    target_link_libraries(
      ${testname}
      PUBLIC
      mantissa
    )
    add_test(
      NAME ${test}
      COMMAND $<TARGET_FILE:${testname}>
      WILL_FAIL
    )
    set_property(
      TEST ${test}
      PROPERTY WILL_FAIL true
    )
  endforeach()
endif()
