//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/path.h"
#include "core/types.h"

class Allocator_t;

enum E_shader_stage {
  e_shader_stage_vertex = 1 << 0,
  e_shader_stage_fragment = 1 << 1,

  e_shader_stage_all = 0xffff
};

enum E_resource_state {
  e_resource_state_none = 0,
  e_resource_state_undefined = 1 << 0,
  e_resource_state_render_target = 1 << 1,
  e_resource_state_depth_write = 1 << 2,
  e_resource_state_depth_read = 1 << 3,
  e_resource_state_present = 1 << 4,
  e_resource_state_pixel_shader_resource = 1 << 5,
};

enum E_render_target_type {
  e_render_target_type_color,
  e_render_target_type_depth_stencil,

  e_render_target_type_count,
};

enum E_format {
  e_format_r32g32b32a32_float,
};

enum E_render_pass_hint {
  e_render_pass_hint_none,
  e_render_pass_hint_shadow,
};

struct Resources_set_create_info_t {
  U8 binding;
  U8 uniform_buffer_count;
  U8 sampler_count;
  U8 image_count;
  E_shader_stage visibility;
};

struct Resources_set_t {
};

struct Pipeline_layout_create_info_t {
  int set_count;
  Resources_set_t** sets;
};

struct Pipeline_layout_t {
};

struct Vertex_buffer_create_info_t {
  Sz size;
  Sz alignment = 256;
  Sz stride;
};

struct Vertex_buffer_t {
  Sz stride;
  void* p;
};
struct Depth_stencil_create_info_t {
  bool can_be_sampled;
};

struct Render_target_t {
  E_render_target_type type;
  E_resource_state state;
};

struct Render_target_description_t {
  Render_target_t* render_target;
  E_resource_state render_pass_state;
  E_resource_state state_after;
};

struct Render_pass_create_info_t {
  int render_target_count;
  Render_target_description_t* descs;
  E_render_pass_hint hint;
  bool is_last;
};

struct Render_pass_t {
  Dynamic_array_t<Render_target_description_t> rt_descs;
  bool is_last;
};

struct Sampler_create_info_t {
  Resources_set_t* resources_set;
  int binding;
};

struct Sampler_t {
};

struct Uniform_buffer_create_info_t {
  Resources_set_t* set;
  Sz size;
  Sz alignment = 256;
  U8 index;
};

struct Uniform_buffer_t {
  void* p;
};

struct Shader_create_info_t {
  Path_t path;
};

struct Shader_t {
};

struct Input_element_t {
  const char* semantic_name = NULL;
  E_format format;
  U8 semantic_index;
  U8 input_slot;
  Sz stride;
};

struct Pipeline_state_object_create_info_t {
  Shader_t* vs = NULL;
  Shader_t* ps = NULL;
  U8 input_element_count = 0;
  Input_element_t* input_elements = NULL;
  Pipeline_layout_t* pipeline_layout = NULL;
  Render_pass_t* render_pass = NULL;
};

struct Pipeline_state_object_t {
};

struct Image_view_create_info_t {
  Resources_set_t* set;
  Render_target_t* render_target;
  E_resource_state state;
  U8 binding;
};

struct Image_view_t {

};

class Gpu_t {
public:
  virtual void destroy() = 0;
  virtual Resources_set_t* create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci);
  virtual Pipeline_layout_t* create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci);
  virtual Vertex_buffer_t* create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) = 0;
  virtual Render_target_t* create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) = 0;
  virtual Render_pass_t* create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci);
  virtual Uniform_buffer_t* create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci);
  virtual Sampler_t* create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci);
  virtual Image_view_t* create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci);
  virtual Shader_t* compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci);
  virtual Pipeline_state_object_t* create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci);
  virtual void get_back_buffer();
  virtual void cmd_begin();
  virtual void cmd_begin_render_pass(Render_pass_t* render_pass);
  virtual void cmd_end_render_pass(Render_pass_t* render_pass);
  virtual void cmd_set_pipeline_state(Pipeline_state_object_t* pso);
  virtual void cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding);
  virtual void cmd_set_uniform_buffer(Uniform_buffer_t* ub, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index);
  virtual void cmd_set_sampler(Sampler_t* sampler, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index);
  virtual void cmd_set_image_view(Image_view_t* image_view, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index);
  virtual void cmd_draw(int vertex_count, int first_vertex);
  virtual void cmd_end();
};