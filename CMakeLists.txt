cmake_minimum_required(VERSION 3.28...3.31)

project(PBB
  LANGUAGES
  C CXX)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMake)
include(CMakeDependentOption)
include(GenerateExportHeader)

option(PBB_HEADER_ONLY "Build as header-only library" ON)

cmake_dependent_option(
  BUILD_SHARED_LIBS
  "Build shared libraries (ignored when header-only)"
  OFF
  "NOT PBB_HEADER_ONLY"
  OFF
)

add_library(build INTERFACE)

include(spsCompilerPlatformFlags)
include(spsAsan)
include(spsInstallDirs)

if(PBB_HEADER_ONLY)
  set(PBB_LIB_TYPE INTERFACE)
elseif(BUILD_SHARED_LIBS)
  set(PBB_LIB_TYPE SHARED)
else()
  set(PBB_LIB_TYPE STATIC)
endif()


include(CTest)
find_package(Catch2 3.5.2 REQUIRED CONFIG)
include(Catch)
enable_testing()

find_package(TBB REQUIRED)
set(TBB_LIBRARIES /opt/intel/oneapi/tbb/2022.0/lib/libtbb.so)

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

