
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

catch_discover_tests(MRMWQueueTest)
catch_discover_tests(PhoenixSingletonTest)
catch_discover_tests(MeyersSingletonTest)
# inline std::shared_ptr<ThreadPool> GetThreadPoolPtr()
# set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# 
# /OPT:REF and /OPT:ICF # remove unused functions
# nm / dumpbin /symbols
# size, objdump
