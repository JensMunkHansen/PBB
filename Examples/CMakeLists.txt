cmake_minimum_required(VERSION 3.28...3.31)
project(PBBExamples)

if (PBB_BINARY_DIR)
  message("Examples tested from ${PBB_BINARY_DIR}")
  function (add_example dir)
    if (1) # pbb_binary_dir (TODO: Define stuff in top-level
      add_test(
        NAME    "PBBExample-${dir}"
        COMMAND "${CMAKE_COMMAND}"
                "-Dconfig=$<CONFIGURATION>"
                "-Dgenerator=${CMAKE_GENERATOR}"
                "-Dsource=${CMAKE_CURRENT_SOURCE_DIR}"
                "-Dbinary=${CMAKE_CURRENT_BINARY_DIR}"
                "-Dexample_dir=${dir}"
                "-Dbuild_type=${CMAKE_BUILD_TYPE}"
                "-Dshared=${BUILD_SHARED_LIBS}"
                "-Dpbb_dir=${PBB_BINARY_DIR}/PBB"
                "-Dctest=${CMAKE_CTEST_COMMAND}"
                "-Dplatform=${CMAKE_GENERATOR_PLATFORM}"
                "-Dtoolset=${CMAKE_GENERATOR_TOOLSET}"
                "-Dpbb_binary_dir=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
                -P "${CMAKE_CURRENT_LIST_DIR}/RunExample.cmake")
      set_property(TEST "PBBExample-${dir}" APPEND
        PROPERTY
          SKIP_REGULAR_EXPRESSION "Skipping example")
    endif ()
  endfunction ()
else ()
  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

add_example(Linux/Usage)
