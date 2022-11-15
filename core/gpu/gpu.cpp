//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/gpu/gpu.h"

#include "core/command_line.h"
#if M_os_is_win()
#include "core/gpu/d3d12/d3d12.h"
#endif
#include "core/gpu/vulkan/vulkan.h"
#include "core/loader/dds.h"
#include "core/log.h"
#include "core/window/window.h"

Texture_create_info_t get_texture_create_info(const Dds_loader_t& dds) {
  Texture_create_info_t ci = {};
  ci.data = dds.m_data;
  ci.width = dds.m_header->width;
  ci.height = dds.m_header->height;
  ci.format = dds.m_format;
  if (ci.format == e_format_bc7_unorm) {
    ci.row_pitch = ci.width * 4;
    ci.row_count = ci.height / 4;
  } else {
    M_unimplemented();
  }
  return ci;
}

Gpu_t* Gpu_t::init(Allocator_t* allocator, Window_t* window) {
  Gpu_t* rv = NULL;
  if (g_cl->get_flag_value("--gpu").get_string().equals("dx12")) {
#if M_os_is_win()
    auto d3d12_gpu = allocator->construct<D3d12_t>();
    d3d12_gpu->init(window);
    rv = d3d12_gpu;
#endif
  } else {
    auto vk_gpu = allocator->construct<Vulkan_t>();
    vk_gpu->init(window);
    rv = vk_gpu;
  }
  return rv;
}

Texture_t* Gpu_t::create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Texture_t* Gpu_t::create_texture_cube(Allocator_t* allocator, const Texture_create_info_t& ci) {
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

Index_buffer_t* Gpu_t::create_index_buffer(Allocator_t* allocator, const Index_buffer_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Render_pass_t* Gpu_t::create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) {
  M_unimplemented();
  return NULL;
}

Resource_t Gpu_t::create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) {
  M_unimplemented();
  return Resource_t();
}

Resource_t Gpu_t::create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) {
  M_unimplemented();
  return Resource_t();
}

Resource_t Gpu_t::create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) {
  M_unimplemented();
  return Resource_t();
}

void Gpu_t::bind_resource_to_set(const Resource_t& resource, const Resources_set_t* set, int binding) {
  M_unimplemented();
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

void Gpu_t::cmd_set_index_buffer(Index_buffer_t* ib) {
  M_unimplemented();
}

void Gpu_t::cmd_set_resource(const Resource_t& resource, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  M_unimplemented();
}

void Gpu_t::cmd_draw(int vertex_count, int first_vertex) {
  M_unimplemented();
}

void Gpu_t::cmd_draw_index(int index_count, int instance_count, int first_index, int vertex_offset, int first_instance) {
  M_unimplemented();
}

void Gpu_t::cmd_set_viewport() {
  Viewport_t viewport = {};
  viewport.top_left_x = 0.0f;
  viewport.top_left_y = 0.0f;
  viewport.width = (float)m_window->m_width;
  viewport.height = (float)m_window->m_height;
  viewport.min_depth = 0.0f;
  viewport.max_depth = 1.0f;
  cmd_set_viewport(1, &viewport);
}

void Gpu_t::cmd_set_viewport(int viewport_count, const Viewport_t* viewports) {
  M_unimplemented();
}

void Gpu_t::cmd_set_scissor() {
  Scissor_t scissor = {};
  scissor.x = 0.0f;
  scissor.y = 0.0f;
  scissor.width = (float)m_window->m_width;
  scissor.height = (float)m_window->m_height;
  cmd_set_scissor(1, &scissor);
}

void Gpu_t::cmd_set_scissor(int count, const Scissor_t* scissors) {
  M_unimplemented();
}

void Gpu_t::cmd_end() {
  M_unimplemented();
}

void Gpu_t::on_resized() {
  M_unimplemented();
}

void Gpu_t::resize_render_pass(Render_pass_t* render_pass) {
  M_unimplemented();
}

int Gpu_t::convert_format_to_size_(E_format format) {
  switch(format) {
    case e_format_r32g32b32a32_float:
      return 16;
    case e_format_r32g32b32a32_uint:
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
    case e_format_bc7_unorm:
    case e_format_bc7_typeless:
      return 4;
    default:
      M_unimplemented();
  }
  return 0;
}

