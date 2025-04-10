cmake_minimum_required(VERSION 3.28...3.31)

project(PBB
  VERSION 1.0.0
  LANGUAGES
  C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake)

include(CMakeDependentOption)

# On Windows, this is stored in the registry. On other platforms,
# inside $HOME/.cmake/packages
option(${PROJECT_NAME}_EXPORT_TO_PACKAGE_REGISTRY "Export installation to package registry" OFF)
if (${PROJECT_NAME}_EXPORT_TO_PACKAGE_REGISTRY)
  set(CMAKE_EXPORT_PACKAGE_REGISTRY 1)
endif()

# === Build mode selection ===
set(${PROJECT_NAME}_LIBRARY_TYPE "INTERFACE" CACHE STRING "Library type to build: INTERFACE (header-only), STATIC, or SHARED")
set_property(CACHE ${PROJECT_NAME}_LIBRARY_TYPE PROPERTY STRINGS INTERFACE STATIC SHARED)

# === Enforce BUILD_SHARED_LIBS based on PBB_LIBRARY_TYPE ===
if(${PROJECT_NAME}_LIBRARY_TYPE STREQUAL "SHARED")
  set(BUILD_SHARED_LIBS ON CACHE BOOL "Use shared libraries" FORCE)
elseif(${PROJECT_NAME}_LIBRARY_TYPE STREQUAL "STATIC" OR ${PROJECT_NAME}_LIBRARY_TYPE STREQUAL "INTERFACE")
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "Use shared libraries" FORCE)
else()
  message(FATAL_ERROR "Invalid PBB_LIBRARY_TYPE: must be INTERFACE, STATIC, or SHARED")
endif()

option(BUILD_EXAMPLES "Build examples consuming the library" ON)
option(BUILD_TESTING "Build testing" ON)
option(PBB_USE_TBB_QUEUE "Use TBB queue" OFF)
option(PBB_USE_TBB_MAP "Use TBB map" OFF)

set(DEBUG_POSTFIX d)

# === Interface target for build ===
add_library(build INTERFACE)
add_library(PBB::build ALIAS build)

include(spsCompilerPlatformFlags)
include(spsAsan)
include(spsInstallDirs)
include(spsConfig)
include(spsCompilerPlatformSupport)
# set(SPS_PEDANTIC_BUILD_WARNINGS 1) #
include(spsCompilerWarningFlags)
include(spsTargets)

if (${PROJECT_NAME}_LIBRARY_TYPE STREQUAL "INTERFACE")
  set(${PROJECT_NAME}_HEADER_ONLY ON)
else()
  set(${PROJECT_NAME}_HEADER_ONLY OFF)
endif()

if (BUILD_TESTING)
  include(CTest)
  find_package(Catch2 3.5.2 REQUIRED CONFIG)
  include(Catch)
  enable_testing()
endif()

find_package(TBB QUIET)
if (NOT TBB_Found)
  set(CANDIDATE /opt/intel/oneapi/tbb/2022.1/lib/libtbb.so)
  if (EXISTS ${CANDIDATE})
    set(TBB_LIBRARIES ${CANDIDATE})
    set(TBB_Found TRUE)
  else()
    set(TBB_Found FALSE)
  endif()
endif()

if (NOT TBB_Found)
  set(PBB_USE_TBB_QUEUE OFF)
  set(PBB_USE_TBB_MAP OFF)
endif()
message("Using TBB Queue: ${PBB_USE_TBB_QUEUE}")
message("Using TBB Map: ${PBB_USE_TBB_MAP}")

if (NOT DEFINED pbb_cmake_build_dir)
  set(pbb_cmake_build_dir
    ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME})
endif()

add_subdirectory(PBB)

find_program(VALGRIND "valgrind")
if (VALGRIND)
  set(DART_CONFIG DartConfiguration.tcl)
  add_custom_target(${DART_CONFIG}
    COMMAND echo "MemoryCheckCommand: ${VALGRIND}" >> ${DART_CONFIG}
    COMMENT "Generating ${DART_CONFIG}"
  )
  set(VALGRIND_ARGS
    --leak-check=full
    --error-exitcode=255
  )
  set(LOGFILE memcheck.log)
  add_custom_target(valgrind
    COMMAND ctest -O ${LOGFILE} -D ExperimentalMemCheck --overwrite MemoryCheckCommandOptions="${VALGRIND_ARGS}"
    COMMAND tail -n1 ${LOGFILE} | grep 'Memory checking results:' > /dev/null
    COMMAND rm -f ${LOGFILE}
    DEPENDS ${DART_CONFIG})
endif()

# Some generated code has some warnings that we need to exclude for now.
configure_file(
  "${PBB_SOURCE_DIR}/.clang-tidy"
  "${PBB_BINARY_DIR}/.clang-tidy"
  COPYONLY)

if (BUILD_EXAMPLES)
  add_subdirectory(Examples)
endif()

include(spsCompileCommands)
