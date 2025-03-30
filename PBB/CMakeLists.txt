include(spsExportHeader)

#set(PBB_USE_TBB_MAP FALSE)
set(TARGET_NAME PBB)
add_library(${TARGET_NAME} ${PBB_LIB_TYPE})

generate_export_header(${TARGET_NAME})
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h @ONLY)

# TODO: Use BUILD_INTERFACE and INSTALL_INTERFACE (more flexible) 
target_sources(${TARGET_NAME}
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
      Memory.hpp
      MeyersSingleton.hpp        
      MRMWQueue.hpp
      ParallelFor.hpp
      ThreadPool.hpp
      ThreadPool.inl
      ThreadPool.txx
      ThreadLocal.hpp
  PRIVATE
)

target_sources(${TARGET_NAME}
  PUBLIC
    FILE_SET generated_headers
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/..
    FILES ${CMAKE_CURRENT_BINARY_DIR}/../PBB/Config.h
)

# If compiled, include the .cpp source for the singleton
if(NOT PBB_HEADER_ONLY)
  target_sources(${TARGET_NAME}
    PUBLIC
      FILE_SET library_sources
      TYPE HEADERS
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
      FILES
        ThreadPoolSingleton.hpp
    PRIVATE
      PhoenixSingleton.hpp        
      ThreadPoolSingleton.cxx
      ThreadPool.cxx
  )
  target_link_libraries(${TARGET_NAME} PUBLIC build)
  if (WIN32 AND BUILD_SHARED_LIBS)
    target_sources(${TARGET_NAME}
      PRIVATE
        WinDllMain.cxx)
  endif()
endif()

# Add compile definition for headers to switch between inline/non-inline
#target_compile_definitions(${TARGET_NAME}
#  INTERFACE $<$<BOOL:${PBB_HEADER_ONLY}>:PBB_HEADER_ONLY>
#)

install(TARGETS ${TARGET_NAME}
  FILE_SET public_headers DESTINATION include
)  

add_subdirectory(Testing/Cxx)
