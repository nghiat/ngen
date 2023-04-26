include(${CMAKE_SOURCE_DIR}/cmake/dxc.cmake)
add_library(core STATIC
  allocator.cpp
  allocator.h
  allocator_internal.cpp
  allocator_internal.h
  bit_stream.cpp
  bit_stream.h
  build.h
  command_line.cpp
  command_line.h
  compiler.h
  core.natvis
  core_allocators.cpp
  core_allocators.h
  core_init.cpp
  core_init.h
  debug.h
  dynamic_array.h
  dynamic_array.inl
  dynamic_lib.h
  file.cpp
  file.h
  fixed_array.h
  fixed_array.inl
  free_list_allocator.cpp
  free_list_allocator.h
  gpu/gpu.cpp
  gpu/gpu.h
  gpu/vulkan/vulkan.cpp
  gpu/vulkan/vulkan.h
  gpu/vulkan/vulkan_loader.cpp
  gpu/vulkan/vulkan_loader.h
  hash.cpp
  hash.h
  hash_table.h
  hash_table.inl
  hash_table2.h
  hash_table2.inl
  intrusive_list.h
  intrusive_list.inl
  linear_allocator.h
  linear_allocator.inl
  loader/dae.cpp
  loader/dae.h
  loader/dds.cpp
  loader/dds.h
  loader/obj.cpp
  loader/obj.h
  loader/png.cpp
  loader/png.h
  loader/tga.cpp
  loader/tga.h
  loader/ttf.cpp
  loader/ttf.h
  loader/xml.cpp
  loader/xml.h
  log.cpp
  log.h
  math/float.h
  math/float.inl
  math/gdb_math.py
  math/mat4.h
  math/mat4.inl
  math/math.natvis
  math/plane.h
  math/quat.h
  math/quat.inl
  math/ray.h
  math/ray.inl
  math/README.md
  math/sphere.h
  math/transform.h
  math/transform.inl
  math/triangle.h
  math/vec2.h
  math/vec2.inl
  math/vec3.h
  math/vec3.inl
  math/vec4.h
  math/vec4.inl
  mono_time.h
  os.h
  path.cpp
  path.h
  path_utils.cpp
  path_utils.h
  string.h
  string.inl
  string_utils.cpp
  string_utils.h
  string_utils_char.cpp
  string_utils_wchar.cpp
  thread.h
  types.h
  utils.h
  value.cpp
  value.h
  window/input.h
  window/window.h
  windows_lite.h)

if (WIN32)
  target_sources(core PRIVATE
    debug_win.cpp
    dynamic_lib_win.cpp
    file_win.cpp
    gpu/d3d12/d3d12.cpp
    gpu/d3d12/d3d12.h
    mono_time_win.cpp
    path_utils_win.cpp
    path_win.cpp
    thread_win.cpp
    window/window_win.cpp
  )
  target_link_libraries(core Dbghelp User32 D3D12 D3DCompiler DXGI)
  target_compile_definitions(core PUBLIC VK_USE_PLATFORM_WIN32_KHR)
  target_link_options(core PUBLIC "/NATVIS:${CMAKE_CURRENT_SOURCE_DIR}/core.natvis")
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_sources(core PRIVATE
    debug_linux.cpp
    dynamic_lib_linux.cpp
    file_linux.cpp
    mono_time_linux.cpp
    path_linux.cpp
    path_utils_linux.cpp
    thread_unix.cpp
    window/window_x11.cpp
  )
  target_compile_definitions(core PUBLIC VK_USE_PLATFORM_XCB_KHR)
  target_link_libraries(core X11 xcb X11-xcb xcb-keysyms)
endif()

target_compile_definitions(core PUBLIC
  "GPU_VK_UNIFORM_BINDING_OFFSET=${uniform_binding_offset}"
  "GPU_VK_TEXTURE_BINDING_OFFSET=${texture_binding_offset}"
  "GPU_VK_SAMPLER_BINDING_OFFSET=${sampler_binding_offset}"
)
