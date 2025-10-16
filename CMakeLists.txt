cmake_minimum_required(VERSION 3.20)
project(sessions VERSION 0.6.0)

option(SESSIONS_TESTS "Build tests." Off)

if(UNIX)
  option(SESSIONS_NOEXTENTIONS "Disable use of the gnu::constructor attribute.")
  if(EXISTS /proc/self/cmdline)
    set(HAS_PROCFS YES)
  endif()
endif()
if(WIN32)
  option(SESSIONS_UTF8 "Use UTF-8 codepage when converting to/from wide strings." On)
endif()

set(INCLUDE include/red/sessions)
set(HEADERS ${INCLUDE}/session.hpp ${INCLUDE}/config.h)

add_library(sessions src/session.cpp ${HEADERS})
target_compile_features(sessions PUBLIC cxx_std_20)

target_include_directories(sessions PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# 3rd-party
find_package(range-v3 CONFIG REQUIRED)
target_link_libraries(sessions PUBLIC range-v3::range-v3)
find_package(Catch2 CONFIG)

if(SESSIONS_TESTS AND Catch2_FOUND)
  enable_testing()

  add_executable(tests test/test.cpp)
  target_link_libraries(tests PRIVATE sessions Catch2::Catch2)
  if(WIN32)
    target_compile_definitions(tests PRIVATE UNICODE)
  endif()

  add_test(environment tests "[env],[var]")
  add_test(arguments   tests "[args]" -- áéíóú words something -l 123)
  add_test(join_paths  tests "join_paths")
endif()

configure_file(config.h.in ${CMAKE_CURRENT_SOURCE_DIR}/${INCLUDE}/config.h)

# installation
add_library(red::sessions ALIAS sessions)

include(cmake/install.cmake)

install_targets(sessions)
configure_and_install(cmake/redConfig.cmake red AnyNewerVersion)
install(DIRECTORY include/ DESTINATION include)

set_target_properties(sessions PROPERTIES DEBUG_POSTFIX d)