cmake_minimum_required(VERSION 3.10)
project(sessions VERSION 0.0.5)

option(SESSIONS_TESTS "Build tests" On)

if(WIN32)
  add_compile_definitions(UNICODE _UNICODE)
else()
  option(SESSION_NOEXTENTIONS "Disable use of the gnu constructor attribute (requires calling 'init_args')")
endif()

set(HEADERS include/session.hpp)

add_library(sessions ${HEADERS} src/session.cpp)

target_include_directories(sessions PUBLIC include)
target_compile_features(sessions PUBLIC cxx_std_20)

# submodules
add_subdirectory(3rd-party/ranges)
target_link_libraries(sessions PUBLIC range-v3::range-v3)


if(SESSIONS_TESTS)
  enable_testing()
  add_executable(tests test/test.cpp)
  target_link_libraries(tests PRIVATE sessions)
  target_include_directories(tests PRIVATE src)
  add_test(
    NAME test-environment
    COMMAND tests "[environment]"
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
  add_test(
    NAME test-arguments
    COMMAND tests "[arguments]" --test-args doh reh mi fa soh
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()

if(SESSION_NOEXTENTIONS)
  target_compile_definitions(sessions PRIVATE SESSION_NOEXTENTIONS)
endif()
