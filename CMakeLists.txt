cmake_minimum_required(VERSION 3.10)
project(sessions)

set(HEADERS include/red/session.hpp include/red/session_impl.hpp src/impl.hpp)

add_library(sessions ${HEADERS} src/session.cpp)
target_sources(sessions PRIVATE
  $<$<NOT:$<PLATFORM_ID:Windows>>:src/posix.cxx>
  $<$<PLATFORM_ID:Windows>:src/windows.cxx>
)
target_include_directories(sessions PUBLIC include)
target_compile_features(sessions PUBLIC cxx_std_17)

# tests
add_executable(tests test/test.cpp test/util.cpp)
target_link_libraries(tests PRIVATE sessions)
target_include_directories(tests PRIVATE src)

if(WIN32)
  add_compile_definitions(UNICODE _UNICODE)
endif()
