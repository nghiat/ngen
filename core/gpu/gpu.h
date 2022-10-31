//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/fixed_array.h"
#include "core/path.h"
#include "core/types.h"

class Allocator_t;
class Command_line_t;
class Dds_loader_t;
class Window_t;

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
  e_format_r32g32b32a32_uint,
  e_format_r32g32b32_float,
  e_format_r32g32_float,
  e_format_r8_uint,
  e_format_r8_unorm,
  e_format_r8g8b8a8_uint,
  e_format_r8g8b8a8_unorm,
  e_format_r16_uint,
  e_format_r16_unorm,
  e_format_r16g16b16a16_uint,
  e_format_r16g16b16a16_unorm,
  e_format_r24_unorm_x8_typeless,
  e_format_bc7_unorm,
  e_format_bc7_typeless,
};

enum E_render_pass_hint {
  e_render_pass_hint_none,
  e_render_pass_hint_shadow,
};

enum E_resource_type {
  e_resource_type_none,
  e_resource_type_uniform_buffer,
  e_resource_type_image_view,
  e_resource_type_sampler,
};

enum E_topology {
  e_topology_triangle,
  e_topology_line,
};

struct Texture_create_info_t {
  U8* data;
  U32 width;
  U32 height;
  U32 row_pitch;
  U32 row_count;
  E_format format;
};

struct Texture_t {
  bool is_cube = false;
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

struct Index_buffer_create_info_t {
  Sz size;
};

struct Vertex_buffer_t {
  Sz stride;
  void* p;
};

struct Index_buffer_t {
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
  bool use_swapchain_render_target;
  bool is_last;
  bool should_clear_render_target;
};

struct Render_pass_t {
  Fixed_array_t<Render_target_description_t, 8> rt_descs;
  bool use_swapchain_render_target;
  bool is_last;
  bool should_clear_render_target;
};

struct Sampler_create_info_t {
};

struct Sampler_t {
};

struct Uniform_buffer_create_info_t {
  Sz size;
  Sz alignment = 256;
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
  const char* semantic_name;
  E_format format;
  // 0 and 1 mean the same
  U8 matrix_row_count;
};

struct Input_slot_t {
  Sz stride;
  U8 slot_num;
  U8 input_element_count = 0;
  Input_element_t* input_elements;
};

struct Pipeline_state_object_create_info_t {
  Shader_t* vs = NULL;
  Shader_t* ps = NULL;
  Input_slot_t* input_slots;
  Pipeline_layout_t* pipeline_layout = NULL;
  Render_pass_t* render_pass = NULL;
  E_topology topology = e_topology_triangle;
  U8 input_slot_count = 0;
  bool enable_depth = false;
};

struct Pipeline_state_object_t {
};

struct Image_view_create_info_t {
  Render_target_t* render_target;
  Texture_t* texture;
  E_resource_state state;
  E_format format;
};

struct Image_view_t {

};

struct Resource_t {
  E_resource_type type = e_resource_type_none;
  union {
    Uniform_buffer_t* uniform_buffer = NULL;
    Image_view_t* image_view;
    Sampler_t* sampler;
  };
};

struct Viewport_t {
  float top_left_x;
  float top_left_y;
  float width;
  float height;
  float min_depth;
  float max_depth;
};

struct Scissor_t {
  U32 x;
  U32 y;
  U32 width;
  U32 height;
};

Texture_create_info_t get_texture_create_info(const Dds_loader_t& dds);

class Gpu_t {
public:
  static Gpu_t* init(Allocator_t* allocator, const Command_line_t* cl, Window_t* window);
  virtual void destroy() = 0;
  virtual Texture_t* create_texture(Allocator_t* allocator, const Texture_create_info_t& ci);
  virtual Texture_t* create_texture_cube(Allocator_t* allocator, const Texture_create_info_t& ci);
  virtual Resources_set_t* create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci);
  virtual Pipeline_layout_t* create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci);
  virtual Vertex_buffer_t* create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) = 0;
  virtual Index_buffer_t* create_index_buffer(Allocator_t* allocator, const Index_buffer_create_info_t& ci);
  virtual Render_target_t* create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) = 0;
  virtual Render_pass_t* create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci);
  virtual Resource_t create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci);
  virtual Resource_t create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci);
  virtual Resource_t create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci);
  virtual void bind_resource_to_set(const Resource_t& resource, const Resources_set_t* set, int binding);
  virtual Shader_t* compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci);
  virtual Pipeline_state_object_t* create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci);

  virtual void get_back_buffer();

  virtual void cmd_begin();
  virtual void cmd_begin_render_pass(Render_pass_t* render_pass);
  virtual void cmd_end_render_pass(Render_pass_t* render_pass);
  virtual void cmd_set_pipeline_state(Pipeline_state_object_t* pso);
  virtual void cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding);
  virtual void cmd_set_index_buffer(Index_buffer_t* ib);
  virtual void cmd_set_resource(const Resource_t& resource, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index);
  virtual void cmd_draw(int vertex_count, int first_vertex);
  virtual void cmd_draw_index(int index_count, int instance_count, int first_index, int vertex_offset, int first_instance);
  virtual void cmd_set_viewport();
  virtual void cmd_set_viewport(int viewport_count, const Viewport_t* viewports);
  virtual void cmd_set_scissor();
  virtual void cmd_set_scissor(int count, const Scissor_t* scissors);
  virtual void cmd_end();

  virtual void on_resized();
  virtual void resize_render_pass(Render_pass_t* render_pass);

  static int convert_format_to_size_(E_format format);

  Window_t* m_window = NULL;
};
