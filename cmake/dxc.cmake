set(uniform_binding_offset 0)
set(texture_binding_offset 20)
set(sampler_binding_offset 40)

function(dxc target)
  if (WIN32)
    set(dxc_exe "${CMAKE_SOURCE_DIR}/third_party/dxc/bin/win64/dxc.exe")
  elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(dxc_exe "${CMAKE_SOURCE_DIR}/third_party/dxc/bin/linux64/bin/dxc")
  endif()
  set(outputs "")
  # input output entry profile
  list(LENGTH ARGN argn_count)
  math(EXPR argn_count "${argn_count} - 1")
  foreach(i RANGE 0 ${argn_count} 4)
    list(GET ARGN ${i} input)
    math(EXPR i "${i} + 1")
    list(GET ARGN ${i} output)
    math(EXPR i "${i} + 1")
    list(GET ARGN ${i} entry)
    math(EXPR i "${i} + 1")
    list(GET ARGN ${i} profile)
    if (WIN32)
      set(output_hlsl ${output}.cso)
      add_custom_command(
          OUTPUT ${output_hlsl}
          COMMAND "${dxc_exe}" -E ${entry} -T ${profile} -Fo ${output_hlsl} -Zi -O1 ${CMAKE_CURRENT_SOURCE_DIR}/${input}
          DEPENDS ${input}
      )
      list(APPEND outputs ${output_hlsl})
    endif()
    set(output_spirv ${output}.spv)
    set(spirv_command "${dxc_exe}" -E ${entry} -T ${profile} -Fo ${output_spirv} -Zi -O1 ${CMAKE_CURRENT_SOURCE_DIR}/${input})
    list(APPEND spirv_command -spirv)
    list(APPEND spirv_command -fvk-b-shift ${uniform_binding_offset} all)
    list(APPEND spirv_command -fvk-t-shift ${texture_binding_offset} all)
    list(APPEND spirv_command -fvk-s-shift ${sampler_binding_offset} all)
    add_custom_command(
        OUTPUT ${output_spirv}
        COMMAND ${spirv_command}
        DEPENDS ${input}
    )
    list(APPEND outputs ${output_spirv})
  endforeach()
  add_custom_target(${target} DEPENDS ${outputs})
endfunction()
