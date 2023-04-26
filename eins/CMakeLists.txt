include(${CMAKE_SOURCE_DIR}/cmake/dxc.cmake)
add_executable(eins
  cam.cpp
  cam.h
  eins.cpp
)
target_link_libraries(eins core)
dxc(eins_shaders
  cube.hlsl cube_vs VSMain vs_5_0
  cube.hlsl cube_ps PSMain ps_5_0
  shadow.hlsl shadow_vs VSMain vs_5_0
  shader.hlsl shader_vs VSMain vs_5_0
  shader.hlsl shader_ps PSMain ps_5_0
  pbr.hlsl pbr_vs VSMain vs_5_0
  pbr.hlsl pbr_ps PSMain ps_5_0
)
add_dependencies(eins assets_copy assets_texture eins_shaders)
