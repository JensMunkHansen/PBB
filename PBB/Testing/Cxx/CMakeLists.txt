function(add_cxx_test target)
  add_executable(${target} "${target}.cxx")
  target_link_libraries(${target} PRIVATE Catch2::Catch2WithMain)
  target_link_libraries(${target} PRIVATE PBB)
  target_link_libraries(${target} PRIVATE PBB::build)
  if (PBB_USE_TBB_MAP OR PBB_USE_TBB_QUEUE)
    target_link_libraries(${target} PRIVATE ${TBB_LIBRARIES})
  endif()
  catch_discover_tests(${target})
endfunction()

add_cxx_test(MRMWQueueTest)
add_cxx_test(PhoenixSingletonTest)
add_cxx_test(MeyersSingletonTest)
add_cxx_test(ThreadPoolTest)

add_cxx_test(ParallelForTest)
add_cxx_test(ThreadLocalTest)

if (BUILD_SHARED_LIBS)
  add_cxx_test(ThreadPoolSingletonTest)
endif()



# inline std::shared_ptr<ThreadPool> GetThreadPoolPtr()
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# 
# /OPT:REF and /OPT:ICF # remove unused functions
# nm / dumpbin /symbols
# size, objdump
