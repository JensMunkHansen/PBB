# InstallPBB.cmake
if (NOT DEFINED config)
  message(FATAL_ERROR "Missing -Dconfig")
endif()

execute_process(
  COMMAND ${CMAKE_COMMAND} --install "${PBB_BINARY_DIR}" --prefix "${PBB_INSTALL_DIR}" --config "${config}"
  RESULT_VARIABLE res
)

if (res)
  message(FATAL_ERROR "Install failed with code ${res}")
endif()
