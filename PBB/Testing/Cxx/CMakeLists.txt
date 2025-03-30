add_executable(MRMWQueueTest MRMWQueueTest.cxx)
target_link_libraries(MRMWQueueTest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(MRMWQueueTest PRIVATE PBB)
target_link_libraries(MRMWQueueTest PUBLIC build)

add_executable(PhoenixSingletonTest PhoenixSingletonTest.cxx)
target_link_libraries(PhoenixSingletonTest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(PhoenixSingletonTest PRIVATE PBB)
target_link_libraries(PhoenixSingletonTest PUBLIC build)

add_executable(MeyersSingletonTest MeyersSingletonTest.cxx)
target_link_libraries(MeyersSingletonTest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(MeyersSingletonTest PRIVATE PBB)
target_link_libraries(MeyersSingletonTest PUBLIC build)

add_executable(ThreadPoolTest ThreadPoolTest.cxx)
target_link_libraries(ThreadPoolTest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(ThreadPoolTest PRIVATE PBB)
target_link_libraries(ThreadPoolTest PUBLIC build)

catch_discover_tests(MRMWQueueTest)
catch_discover_tests(PhoenixSingletonTest)
catch_discover_tests(MeyersSingletonTest)
catch_discover_tests(ThreadPoolTest)

if (1)
add_executable(ThreadPoolTBBTest ThreadPoolTest.cxx)
target_link_libraries(ThreadPoolTBBTest PRIVATE Catch2::Catch2WithMain)
target_link_libraries(ThreadPoolTBBTest PRIVATE PBB)
target_link_libraries(ThreadPoolTBBTest PUBLIC build)
target_compile_definitions(ThreadPoolTBBTest PRIVATE PBB_USE_TBB_QUEUE=1)
target_link_libraries(ThreadPoolTBBTest PRIVATE ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(ThreadPoolTBBTest PRIVATE ${TBB_LIBRARIES})
catch_discover_tests(ThreadPoolTBBTest)
endif()

# Only apply this if we're using Clang
if (0)#CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(STATUS "Clang detected — configuring ASan LD_PRELOAD logic")

  # Ask the compiler where its ASan runtime is
  execute_process(
    COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=libclang_rt.asan-x86_64.so
    OUTPUT_VARIABLE ASAN_RUNTIME_PATH
    OUTPUT_STRIP_TRAILING_WHITESPACE
  )

  if (EXISTS "${ASAN_RUNTIME_PATH}")
    message(STATUS "ASan runtime found at: ${ASAN_RUNTIME_PATH}")

    # Apply to each discovered test
    catch_discover_tests(MeyersSingletonTest)

    foreach(test_name IN LISTS CATCH_DISCOVERED_TESTS)
      set_tests_properties(${test_name} PROPERTIES
        ENVIRONMENT "$<IF:$<CONFIG:Asan>,LD_PRELOAD=${ASAN_RUNTIME_PATH},>"
      )
    endforeach()
  else()
    message(WARNING "ASan runtime not found — LD_PRELOAD will not be set.")
  endif()
endif()



# inline std::shared_ptr<ThreadPool> GetThreadPoolPtr()
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# 
# /OPT:REF and /OPT:ICF # remove unused functions
# nm / dumpbin /symbols
# size, objdump
