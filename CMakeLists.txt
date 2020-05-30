cmake_minimum_required(VERSION 3.10)
project(sessions VERSION 0.1.2)

option(SESSIONS_TESTS "Build tests" On)

if(WIN32)
  add_compile_definitions(UNICODE _UNICODE)
else()
  option(SESSION_NOEXTENTIONS "Disable use of the gnu constructor attribute (requires calling 'init_args')")
endif()

add_library(sessions src/session.cpp)

target_include_directories(sessions PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(MSVC)
  target_compile_features(sessions PUBLIC cxx_std_20)
else()
  target_compile_features(sessions PUBLIC cxx_std_17)
endif()

# 3rd-party
find_package(range-v3 0.10.0 CONFIG REQUIRED)

target_link_libraries(sessions PUBLIC range-v3::range-v3)

if(SESSIONS_TESTS)
  find_package(Catch2 CONFIG REQUIRED)
  enable_testing()

  add_executable(tests test/test.cpp)
  target_link_libraries(tests PRIVATE sessions Catch2::Catch2)

  add_test(NAME all-tests  COMMAND tests -- áéíóú words something -l 123
          WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

if(SESSION_NOEXTENTIONS)
  target_compile_definitions(sessions PRIVATE SESSION_NOEXTENTIONS)
endif()

# installation
add_library(red::sessions ALIAS sessions)
include(cmake/install.cmake)

install_targets(sessions)
configure_and_install(cmake/redConfig.cmake red:: AnyNewerVersion)