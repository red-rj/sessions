cmake_minimum_required(VERSION 3.12)
project(sessions)

set(INC_DIR ${PROJECT_SOURCE_DIR}/include/ixm)
set(HEADERS ${INC_DIR}/session.hpp ${INC_DIR}/session_impl.hpp)

add_library(sessions ${HEADERS} src/session.cpp)

target_include_directories(sessions PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_compile_features(sessions PUBLIC cxx_std_17)
target_sources(sessions PRIVATE
  $<$<NOT:$<PLATFORM_ID:Windows>>:src/posix.cxx>
  $<$<PLATFORM_ID:Windows>:src/windows.cxx>
)

add_executable(lab test/lab.cpp)
target_link_libraries(lab PRIVATE sessions)
