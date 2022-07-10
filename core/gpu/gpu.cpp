//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/gpu/gpu.h"

#include "core/log.h"

Texture_t* Gpu_t::create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Resources_set_t* Gpu_t::create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Pipeline_layout_t* Gpu_t::create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Render_pass_t* Gpu_t::create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Sampler_t* Gpu_t::create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Image_view_t* Gpu_t::create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Uniform_buffer_t* Gpu_t::create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Shader_t* Gpu_t::compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Pipeline_state_object_t* Gpu_t::create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

void Gpu_t::get_back_buffer() {
  M_unimplemented();
}

void Gpu_t::cmd_begin() {
  M_unimplemented();
}

void Gpu_t::cmd_begin_render_pass(Render_pass_t* render_pass) {
  M_unimplemented();
}

void Gpu_t::cmd_end_render_pass(Render_pass_t* render_pass) {
  M_unimplemented();
}

void Gpu_t::cmd_set_pipeline_state(Pipeline_state_object_t* pso) {
  M_unimplemented();
}

void Gpu_t::cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding) {
  M_unimplemented();
}

void Gpu_t::cmd_set_uniform_buffer(Uniform_buffer_t* ub, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  M_unimplemented();
}

void Gpu_t::cmd_set_sampler(Sampler_t* sampler, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  M_unimplemented();
}

void Gpu_t::cmd_set_image_view(Image_view_t* image_view, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  M_unimplemented();
}

void Gpu_t::cmd_draw(int vertex_count, int first_vertex) {
  M_unimplemented();
}

void Gpu_t::cmd_end() {
  M_unimplemented();
}

int Gpu_t::convert_format_to_size_(E_format format) {
  switch(format) {
    case e_format_r32g32b32a32_float:
      return 16;
    case e_format_r32g32b32_float:
      return 12;
    case e_format_r32g32_float:
      return 8;
    case e_format_r8_uint:
      return 1;
    case e_format_r8_unorm:
      return 4;
    case e_format_r8g8b8a8_uint:
      return 4;
    case e_format_r8g8b8a8_unorm:
      return 4;
    case e_format_r16_uint:
      return 2;
    case e_format_r16_unorm:
      return 2;
    case e_format_r16g16b16a16_uint:
      return 8;
    case e_format_r16g16b16a16_unorm:
      return 8;
    case e_format_r24_unorm_x8_typeless:
      return 4;
    default:
      M_unimplemented();
  }
  return 0;
}

