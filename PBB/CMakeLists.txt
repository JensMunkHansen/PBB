include(spsExportHeader)

set(TARGET_NAME PBB)
add_library(${TARGET_NAME} ${PBB_LIB_TYPE})

sps_generate_custom_export_header(${TARGET_NAME})

target_sources(${TARGET_NAME}
  PUBLIC
    FILE_SET public_headers
    TYPE HEADERS
    BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
    FILES
      MRMWQueue.hpp
      ThreadPool.hpp
      ThreadPool.inl
      ThreadPool.txx
      MeyersSingleton.hpp        
  PRIVATE
    PhoenixSingleton.hpp        
)

# If compiled, include the .cpp source for the singleton
if(NOT PBB_HEADER_ONLY)
  target_sources(${TARGET_NAME}
    PUBLIC
      FILE_SET public_headers
      BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/..
      FILES
        ThreadPoolSingleton.hpp
    PRIVATE
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
target_compile_definitions(${TARGET_NAME}
  INTERFACE $<$<BOOL:${PBB_HEADER_ONLY}>:PBB_HEADER_ONLY>
)

install(TARGETS ${TARGET_NAME}
  FILE_SET public_headers DESTINATION include
)  

add_subdirectory(Testing/Cxx)
