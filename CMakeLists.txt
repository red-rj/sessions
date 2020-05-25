cmake_minimum_required(VERSION 3.10)
project(sessions VERSION 0.1.0)

option(SESSIONS_TESTS "Build tests" On)

if(WIN32)
  add_compile_definitions(UNICODE _UNICODE)
else()
  option(SESSION_NOEXTENTIONS "Disable use of the gnu constructor attribute (requires calling 'init_args')")
endif()

set(HEADERS include/session.hpp include/ranges.hpp include/sys_layer.hpp)

add_library(sessions ${HEADERS} src/session.cpp)

target_include_directories(sessions PUBLIC include)
target_compile_features(sessions PUBLIC cxx_std_20)

# submodules
# add_subdirectory(3rd-party/ranges)
# target_link_libraries(sessions PUBLIC range-v3::range-v3)

# 3rd-party
find_package(Catch2 CONFIG)
find_package(range-v3 0.10.0 CONFIG REQUIRED)

target_link_libraries(sessions PUBLIC range-v3::range-v3)


if(SESSIONS_TESTS AND Catch2_FOUND)
  add_executable(tests test/test.cpp)
  target_link_libraries(tests PRIVATE sessions)
  target_link_libraries(tests PRIVATE Catch2::Catch2)
  target_include_directories(tests PRIVATE src)
endif()

if(SESSION_NOEXTENTIONS)
  target_compile_definitions(sessions PRIVATE SESSION_NOEXTENTIONS)
endif()
