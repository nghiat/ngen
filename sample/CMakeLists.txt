include(${CMAKE_SOURCE_DIR}/cmake/dxc.cmake)
add_executable(dae_sample dae_sample.cpp)
target_link_libraries(dae_sample core)
dxc(sample_shaders
  text.hlsl text_vs VSMain vs_5_0)
