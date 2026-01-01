set(error_header "${CMAKE_CURRENT_BINARY_DIR}/generated/error_codes.h")
add_custom_command(OUTPUT "${error_header}"
  COMMAND ${Python3_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_errors.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/share" "${error_header}"
  DEPENDS cmake/generate_errors.py share/messages_to_clients.txt share/messages_to_error_log.txt
  VERBATIM)
add_custom_target(generate_errors DEPENDS "${error_header}")
