//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/dynamic_array.h"
#include "core/file.h"
#include "core/gpu/vulkan/vulkan.h"
#include "core/linear_allocator.h"
#include "core/loader/ttf.h"
#include "core/math/mat4.h"
#include "core/math/transform.h"
#include "core/math/vec2.h"
#include "core/path.h"
#include "core/path_utils.h"
#include "core/utils.h"
#include "core/window/window.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb/stb_truetype.h"

class Font_window_t : public Window_t {
public:
  Font_window_t() : Window_t(M_txt("Font"), 1024, 768), m_allocator("allocator") {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_resized() override;

  Linear_allocator_t<> m_allocator;
  Gpu_t* m_gpu = NULL;
  Render_pass_t* m_render_pass;
  Resources_set_t* m_ub_set;
  Resources_set_t* m_sampler_set;
  Resources_set_t* m_texture_set;
  Pipeline_layout_t* m_pipeline_layout;
  Resource_t m_uniform;
  Resource_t m_sampler;
  Texture_t* m_texture;
  Resource_t m_srv;
  M4_t* m_world_mat;
  Vertex_buffer_t* m_vb;
  int m_vertex_count = 0;
  Pipeline_state_object_t* m_pso;
};

bool Font_window_t::init() {
  Window_t::init();
  Linear_allocator_t<> temp_allocator("font_init_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  m_gpu = m_allocator.construct<Vulkan_t>();
  ((Vulkan_t*)m_gpu)->init(this);
  Ttf_loader_t ttf(&temp_allocator);
  ttf.init(g_exe_dir.join(M_txt("assets/UbuntuMono-Regular.ttf")));

  {
    Render_pass_create_info_t rp_ci = {};
    rp_ci.use_swapchain_render_target = true;
    rp_ci.should_clear_render_target = true;
    rp_ci.is_last = true;
    m_render_pass = m_gpu->create_render_pass(&m_allocator, rp_ci);
  }
  {
    {
      Resources_set_create_info_t res_set_ci = {};
      res_set_ci.binding = 0;
      res_set_ci.uniform_buffer_count = 1;
      res_set_ci.visibility = e_shader_stage_vertex;
      m_ub_set = m_gpu->create_resources_set(&m_allocator, res_set_ci);
    }
    {
      Resources_set_create_info_t res_set_ci = {};
      res_set_ci.binding = 0;
      res_set_ci.sampler_count = 1;
      res_set_ci.visibility = e_shader_stage_fragment;
      m_sampler_set = m_gpu->create_resources_set(&m_allocator, res_set_ci);
    }
    {
      Resources_set_create_info_t res_set_ci = {};
      res_set_ci.binding = 0;
      res_set_ci.image_count = 1;
      res_set_ci.visibility = e_shader_stage_fragment;
      m_texture_set = m_gpu->create_resources_set(&m_allocator, res_set_ci);
    }
    Resources_set_t* sets[] = {m_ub_set, m_sampler_set, m_texture_set};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = 3;
    ci.sets = sets;
    m_pipeline_layout = m_gpu->create_pipeline_layout(&m_allocator, ci);
  }
  {
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.size = sizeof(M4_t);
    ub_ci.alignment = 256;
    m_uniform = m_gpu->create_uniform_buffer(&m_allocator, ub_ci);
    m_gpu->bind_resource_to_set(m_uniform, m_ub_set, 0);
    m_world_mat = (M4_t*)m_uniform.uniform_buffer->p;
    *m_world_mat = scale(0.001f, 0.001f, 0.001f);
  }
  {
    Sampler_create_info_t sampler_ci = {};
    m_sampler = m_gpu->create_sampler(&m_allocator, sampler_ci);
    m_gpu->bind_resource_to_set(m_sampler, m_sampler_set, 0);
  }
  {
    Texture_create_info_t ci = {};
    ci.data = ttf.m_data;
    ci.width = ttf.m_width;
    ci.height = ttf.m_height;
    ci.format = e_format_r8_uint;
    ci.row_pitch = ci.width;
    ci.row_count = ci.height;
    m_texture = m_gpu->create_texture(&m_allocator, ci);

    Image_view_create_info_t image_view_ci = {};
    image_view_ci.texture = m_texture;
    image_view_ci.format = e_format_r8_unorm;
    m_srv = m_gpu->create_image_view(&m_allocator, image_view_ci);
    m_gpu->bind_resource_to_set(m_srv, m_texture_set, 0);
  }
  {
    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 1024 * 1024;
    vb_ci.alignment = 256;
    vb_ci.stride = 2 * sizeof(float);
    m_vb = m_gpu->create_vertex_buffer(&m_allocator, vb_ci);
    struct Input_t_ {
      V2_t pos;
      V2_t uv;
    };
    Input_t_* vb = (Input_t_*)m_vb->p;
    vb[0] = {(V2_t){ttf.m_x_min, ttf.m_y_min}, (V2_t){0.0f, 1.0f}};
    vb[1] = {(V2_t){ttf.m_x_max, ttf.m_y_max}, (V2_t){1.0f, 0.0f}};
    vb[2] = {(V2_t){ttf.m_x_min, ttf.m_y_max}, (V2_t){0.0f, 0.0f}};
    vb[3] = {(V2_t){ttf.m_x_max, ttf.m_y_max}, (V2_t){1.0f, 0.0f}};
    vb[4] = {(V2_t){ttf.m_x_min, ttf.m_y_min}, (V2_t){0.0f, 1.0f}};
    vb[5] = {(V2_t){ttf.m_x_max, ttf.m_y_min}, (V2_t){1.0f, 1.0f}};
    m_vertex_count = 6;

  }
  {
    Input_element_t input_elems[2] = {};
    input_elems[0].semantic_name = "POSITION";
    input_elems[0].format = e_format_r32g32_float;
    input_elems[1].semantic_name = "UV";
    input_elems[1].format = e_format_r32g32_float;
    Input_slot_t input_slot = {};
    input_slot.stride = 4 * sizeof(float);
    input_slot.slot_num = 0;
    input_slot.input_elements = input_elems;
    input_slot.input_element_count = static_array_size(input_elems);
    Shader_t* text_vs = m_gpu->compile_shader(&m_allocator, {g_exe_dir.join(M_txt("assets/sample/text_vs"))});
    Shader_t* text_ps = m_gpu->compile_shader(&m_allocator, {g_exe_dir.join(M_txt("assets/sample/text_ps"))});
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = text_vs;
    pso_ci.ps = text_ps;
    pso_ci.input_slot_count = 1;
    pso_ci.input_slots = &input_slot;
    pso_ci.pipeline_layout = m_pipeline_layout;
    pso_ci.render_pass = m_render_pass;
    pso_ci.topology = e_topology_triangle;
    m_pso = m_gpu->create_pipeline_state_object(&m_allocator, pso_ci);
  }

  return true;
}

void Font_window_t::destroy() {
}

void Font_window_t::loop() {
  m_gpu->get_back_buffer();
  m_gpu->cmd_begin();
  m_gpu->cmd_set_viewport();
  m_gpu->cmd_set_scissor();
  m_gpu->cmd_begin_render_pass(m_render_pass);
  m_gpu->cmd_set_pipeline_state(m_pso);
  m_gpu->cmd_set_vertex_buffer(m_vb, 0);
  m_gpu->cmd_set_resource(m_uniform, m_pipeline_layout, m_ub_set, 0);
  m_gpu->cmd_set_resource(m_sampler, m_pipeline_layout, m_sampler_set, 1);
  m_gpu->cmd_set_resource(m_srv, m_pipeline_layout, m_texture_set, 2);
  m_gpu->cmd_draw(m_vertex_count, 0);
  m_gpu->cmd_end_render_pass(m_render_pass);
  m_gpu->cmd_end();
}

void Font_window_t::on_resized() {
  if (m_gpu) {
    m_gpu->on_resized();
  }
}

int main(int argc, char** argv) {
  core_init(M_txt("font_sample.log"));

  {
    unsigned char screen[20][79];
    stbtt_fontinfo font;
    int i, j, ascent, baseline, ch = 0;
    float scale, xpos = 2; // leave a little padding in case the character extends left
    char c = 'a';          // intentionally misspelled to show 'lj' brokenness

    Dynamic_array_t<U8> buffer = File_t::read_whole_file_as_binary(g_persistent_allocator, g_exe_dir.join(M_txt("assets/UbuntuMono-Regular.ttf")).m_path);
    stbtt_InitFont(&font, (const unsigned char*)buffer.m_p, 0);

    scale = stbtt_ScaleForPixelHeight(&font, 15);
    stbtt_GetFontVMetrics(&font, &ascent, 0, 0);
    baseline = (int)(ascent * scale);

    int advance, lsb, x0, y0, x1, y1;
    float x_shift = xpos - (float)floor(xpos);
    stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
    stbtt_GetCodepointBitmapBoxSubpixel(&font, c, scale, scale, x_shift, 0, &x0, &y0, &x1, &y1);
    stbtt_MakeCodepointBitmapSubpixel(&font, &screen[baseline + y0][(int)xpos + x0], x1 - x0, y1 - y0, 79, scale, scale, x_shift, 0, c);
  }

  Font_window_t w;
  w.init();
  w.os_loop();
  core_destroy();
  return 0;
}
