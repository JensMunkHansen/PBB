function(add_cxx_test target)
  add_executable(${target} "${target}.cxx")
  target_link_libraries(${target} PRIVATE Catch2::Catch2WithMain)
  target_link_libraries(${target} PRIVATE PBB)
  target_link_libraries(${target} PRIVATE PBB::build)
  if (PBB_USE_TBB_MAP OR PBB_USE_TBB_QUEUE)
    target_link_libraries(${target} PRIVATE ${TBB_LIBRARIES})
    target_link_libraries(${target} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
  endif()
  spsSetDebugPostfix(${target} d)

  if (MSVC)
    foreach(tgt IN ITEMS Catch2::Catch2 Catch2::Catch2Main Catch2::Catch2WithMain)
      if(TARGET ${tgt})
        get_target_property(_dll_path ${tgt} LOCATION)
        if (_dll_path AND EXISTS "${_dll_path}")
          message(STATUS "Copying DLL from ${tgt}: ${_dll_path}")
          add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
	    "$<TARGET_FILE:${tgt}>"
            "$<TARGET_FILE_DIR:${target}>"
            COMMENT "Copying ${_dll_path} to $<TARGET_FILE_DIR:${target}>"
          )
        else()
          message(STATUS "Target ${tgt} exists, but no DLL found at ${_dll_path}")
        endif()
      else()
        message(STATUS "Target ${tgt} not found")
      endif()
    endforeach()
  endif()  
  catch_discover_tests(${target})
endfunction()

add_cxx_test(ThreadPoolTest)
add_cxx_test(ThreadPoolCustomTest)
add_cxx_test(MRMWQueueTest)
add_cxx_test(PhoenixSingletonTest)
add_cxx_test(PhoenixSingletonRefTest)
add_cxx_test(MeyersSingletonTest)

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
