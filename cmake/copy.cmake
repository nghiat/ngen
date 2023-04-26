function(copy target folder)
  set(outputs "")
  # input output entry profile
  foreach(arg ${ARGN})
    set(input ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
    set(output ${folder}/${arg})
    add_custom_command(
      OUTPUT ${output}
      COMMAND ${CMAKE_COMMAND} -E copy ${input} ${output}
      DEPENDS ${input})
    list(APPEND outputs ${output})
  endforeach()
  add_custom_target(${target} DEPENDS ${outputs})
endfunction()
