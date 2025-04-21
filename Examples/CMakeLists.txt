cmake_minimum_required(VERSION 3.28...3.31)
project(PBBExamples)

if (PBB_INSTALL_DIR)
  # Test that the libary can install
  add_custom_target(install-pbb
    COMMAND ${CMAKE_COMMAND}
      -Dconfig=$<CONFIGURATION>
      -DPBB_BINARY_DIR=${PBB_BINARY_DIR}
      -DPBB_INSTALL_DIR=${PBB_INSTALL_DIR}
      -P "${CMAKE_CURRENT_LIST_DIR}/InstallPBB.cmake"
    COMMENT "Installing PBB using CMake script"
    VERBATIM
  )
  add_test(NAME InstallPBB
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install-pbb --config $<CONFIGURATION>)
  set_tests_properties(InstallPBB PROPERTIES FIXTURES_SETUP InstallPBBFixture)
endif()

if (PBB_BINARY_DIR)
  message("Examples tested from ${PBB_BINARY_DIR} and install location: ${PBB_INSTALL_DIR}")
  message("Examples tested from ${PBB_BINARY_DIR}")
  function(_add_example_test test_name dir pbb_dir pbb_binary_dir fixture)
    # Determine unique binary output dir for this test
    if (fixture STREQUAL "InstallPBBFixture")
      set(example_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${dir}-installed")
    else ()
      set(example_binary_dir "${CMAKE_CURRENT_BINARY_DIR}/${dir}")
    endif ()    
    add_test(
      NAME "${test_name}"
      COMMAND "${CMAKE_COMMAND}"
              "-Dconfig=$<CONFIGURATION>"
              "-Dgenerator=${CMAKE_GENERATOR}"
              "-Dsource=${CMAKE_CURRENT_SOURCE_DIR}"
              "-Dbinary=${CMAKE_CURRENT_BINARY_DIR}"
              "-Dexample_dir=${dir}"
              "-Dbuild_type=${CMAKE_BUILD_TYPE}"
              "-Dshared=${BUILD_SHARED_LIBS}"
              "-Dpbb_dir=${pbb_dir}"
              "-Dexample_binary_dir=${example_binary_dir}"              
              "-Dctest=${CMAKE_CTEST_COMMAND}"
              "-Dplatform=${CMAKE_GENERATOR_PLATFORM}"
              "-Dtoolset=${CMAKE_GENERATOR_TOOLSET}"
              "-Dpbb_binary_dir=${pbb_binary_dir}"
              -P "${CMAKE_CURRENT_LIST_DIR}/RunExample.cmake")
  
    set_property(TEST "${test_name}" APPEND
      PROPERTY SKIP_REGULAR_EXPRESSION "Skipping example")
  
    if (fixture)
      set_tests_properties("${test_name}" PROPERTIES FIXTURES_REQUIRED "${fixture}")
    endif ()
  endfunction()
  function(add_example dir)
    if (PBB_BINARY_DIR)
      _add_example_test(
        "PBBExample-${dir}"
        "${dir}"
        "${PBB_BINARY_DIR}/PBB"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
        "")  # no fixture
    endif ()
  endfunction()
  
  function(add_installed_example dir)
    if (PBB_INSTALL_DIR)
      _add_example_test(
        "PBBInstalledExample-${dir}"
        "${dir}"
        "${PBB_INSTALL_DIR}/lib/cmake/PBB"
        "${PBB_INSTALL_DIR}/bin"
        "InstallPBBFixture")  # add fixture
    endif ()
  endfunction()
else ()
  macro (add_example dir)
    add_subdirectory("${dir}")
  endmacro ()
endif ()

add_example(Linux/Usage)
add_installed_example(Linux/Usage)
