function(texture target format)
  if (WIN32)
    set(texture_exe ${CMAKE_SOURCE_DIR}/third_party/compressonator/bin/win64/compressonatorcli.exe)
  else()
    set(texture_exe ${CMAKE_SOURCE_DIR}/third_party/compressonator/bin/linux64/compressonatorcli)
  endif()

  set(outputs "")
  foreach(arg ${ARGN})
    get_filename_component(output ${arg} NAME_WE)
    set(output ${output}.dds)
    add_custom_command(
        OUTPUT ${output}
        COMMAND ${texture_exe} -fd ${format} -EncodeWith GPU ${CMAKE_CURRENT_SOURCE_DIR}/${arg} ${output}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${arg}
    )
    list(APPEND outputs ${output})
  endforeach()
  add_custom_target(${target} DEPENDS ${outputs})
endfunction()
