# Unit tests for the parts of the plugin that run without OBS: the letterbox
# fit math in src/canvas-fit.hpp (plain C++) and the central-widget swap in
# src/central-swap.cpp (Qt Widgets, run on a headless Qt platform so no
# display is needed).
#
# Builds on its own (cmake -S tests -B build_tests) with a C++17 compiler; the
# swap tests also need Qt 6 Widgets on CMAKE_PREFIX_PATH and are skipped when
# it is missing. Or build as part of the plugin with -DENABLE_TESTS=ON.

cmake_minimum_required(VERSION 3.28...3.30)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(borderless-preview-for-obs-tests LANGUAGES CXX)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
  set(CMAKE_CXX_EXTENSIONS FALSE)
  if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
  endif()
  # Let a RelWithDebInfo test build use a Qt that only ships Release binaries.
  # gersemi: off
  set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO RelWithDebInfo Release MinSizeRel None "")
  set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL MinSizeRel Release RelWithDebInfo None "")
  # gersemi: on
  enable_testing()
  find_package(Qt6 COMPONENTS Widgets QUIET)
  include("${CMAKE_CURRENT_SOURCE_DIR}/../cmake/common/qt_wrapopengl_fix.cmake")
  fix_qt_wrapopengl_agl()
endif()

set(_tests_src "${CMAKE_CURRENT_SOURCE_DIR}/../src")

# Angle-bracket includes (Qt) are external to MSVC and left out of /W4; our own headers are quoted.
set(
  _tests_msvc_options
  /W4
  /WX
  /utf-8
  /external:anglebrackets
  /external:W0
)
set(_tests_gnu_options -Wall -Wextra -Wpedantic -Werror)
set(_tests_clang_options -Wno-quoted-include-in-framework-header -Wno-comma)

function(_borderless_test_target target)
  target_include_directories(${target} PRIVATE "${_tests_src}")
  target_compile_features(${target} PRIVATE cxx_std_17)
  target_compile_options(
    ${target}
    PRIVATE
      "$<$<CXX_COMPILER_ID:MSVC>:${_tests_msvc_options}>"
      "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:${_tests_gnu_options}>"
      "$<$<CXX_COMPILER_ID:Clang,AppleClang>:${_tests_clang_options}>"
  )
endfunction()

add_executable(canvas-fit-tests main.cpp test-canvas-fit.cpp)
_borderless_test_target(canvas-fit-tests)
add_test(NAME canvas-fit COMMAND canvas-fit-tests)

if(TARGET Qt6::Widgets)
  add_executable(central-swap-tests main.cpp test-central-swap.cpp "${_tests_src}/central-swap.cpp")
  _borderless_test_target(central-swap-tests)
  target_compile_definitions(central-swap-tests PRIVATE BORDERLESS_TESTS_QT)
  target_link_libraries(central-swap-tests PRIVATE Qt6::Widgets)
  add_test(NAME central-swap COMMAND central-swap-tests)

  # Qt6_DIR is <prefix>/lib/cmake/Qt6; the runtime pieces sit under <prefix>.
  get_filename_component(_qt_prefix "${Qt6_DIR}/../../.." ABSOLUTE)
  if(APPLE)
    # The plugin build turns RPATH handling off and points Xcode at the OBS
    # bundle layout; the test binary needs to find the Qt frameworks instead.
    set_target_properties(
      central-swap-tests
      PROPERTIES
        SKIP_BUILD_RPATH FALSE
        BUILD_RPATH "${_qt_prefix}/lib"
        XCODE_ATTRIBUTE_LD_RUNPATH_SEARCH_PATHS "${_qt_prefix}/lib"
        XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME NO
    )
  elseif(WIN32)
    # Qt DLLs live next to the tools in <prefix>/bin.
    set_tests_properties(central-swap PROPERTIES ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${_qt_prefix}/bin")
  endif()
else()
  message(STATUS "Qt6 Widgets not found; skipping the central-swap tests")
endif()
