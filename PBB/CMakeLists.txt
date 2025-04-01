include(GenerateExportHeader)

set(TARGET_NAME PBB)
add_library(${TARGET_NAME} ${PBB_LIB_TYPE})

if(NOT PBB_HEADER_ONLY)
  generate_export_header(${TARGET_NAME})
endif()

# Configure file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.h.in ${CMAKE_CURRENT_BINARY_DIR}/Config.h @ONLY)

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
      ThreadPoolCommon.hpp
      ThreadPoolTags.hpp
      ThreadPoolBase.hpp
      ThreadPool.hpp
      ThreadPoolTraits.hpp
      ThreadPoolCustom.hpp
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

# If shared or static lib, include explicit instantiation and a
# Phoenix singleton
if(NOT PBB_HEADER_ONLY)
  target_sources(${TARGET_NAME}
    PUBLIC
      FILE_SET library_headers
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
else()
  target_link_libraries(${TARGET_NAME} INTERFACE build)
endif()

# Some IDE's have poor support for FILE_SET
target_include_directories(${TARGET_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# For both build and install tree. Needed for other libraries to use
# find_package(SPSFunction)
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake"
  VERSION ${PBB_VERSION}
  COMPATIBILITY AnyNewerVersion
)

set(LIBRARY_HEADERS_INSTALL_ARG "")
if(NOT PBB_HDR_ONLY)
  set(LIBRARY_HEADERS_INSTALL_ARG
    FILE_SET library_headers DESTINATION include COMPONENT include
  )
endif()

# Define your exports: If we had binaries they will get exported. Other
# targets in the solution can use them
install(TARGETS ${TARGET_NAME} build 
    EXPORT ${TARGET_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Runtime
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT Development
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime
    FILE_SET public_headers DESTINATION include COMPONENT Development
    FILE_SET generated_headers DESTINATION include COMPONENT Development
    ${LIBRARY_HEADERS_INSTALL_ARG}      
    FRAMEWORK COMPONENT Runtime # MacOS (.so and lib)
    PRIVATE_HEADER COMPONENT Development
    BUNDLE DESTINATION bin COMPOENENT Runtime
)  

# Both build and install tree
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/${TARGET_NAME}Config.cmake.in"      # Input
    "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake"         # Build tree
    INSTALL_DESTINATION
      ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME} # Install tree
)

# For the install tree (can be skipped completely)
install(
  EXPORT ${TARGET_NAME}Targets
  NAMESPACE PBB::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME}) # Install
install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}ConfigVersion.cmake" # Already created
    "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Config.cmake"        # Already created
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/${TARGET_NAME})                   # Install tree

# Generate export targets for the build
export(EXPORT ${TARGET_NAME}Targets
  FILE "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}Targets.cmake"
  NAMESPACE PBB::
)

# Export the package for use from the build-tree
# (this registers the build-tree with a global CMake-registry)
# If CMAKE_EXPORT_PACKAGE_REGISTRY == 0, this does nothing (default) 
export(PACKAGE ${TARGET_NAME})

add_subdirectory(Testing/Cxx)
