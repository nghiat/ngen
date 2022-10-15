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

  Linear_allocator_t<> m_allocator;
  Gpu_t* m_gpu;
  Render_pass_t* m_render_pass;
  Resources_set_t* m_resources_set;
  Pipeline_layout_t* m_pipeline_layout;
  Resource_t m_uniform;
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

  {
    Render_pass_create_info_t rp_ci = {};
    rp_ci.use_swapchain_render_target = true;
    rp_ci.should_clear_render_target = true;
    rp_ci.is_last = true;
    m_render_pass = m_gpu->create_render_pass(&m_allocator, rp_ci);
  }
  {
    Resources_set_create_info_t res_set_ci = {};
    res_set_ci.binding = 0;
    res_set_ci.uniform_buffer_count = 1;
    res_set_ci.visibility = e_shader_stage_vertex;
    m_resources_set = m_gpu->create_resources_set(&m_allocator, res_set_ci);
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = 1;
    ci.sets = &m_resources_set;
    m_pipeline_layout = m_gpu->create_pipeline_layout(&m_allocator, ci);
  }
  {
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.size = sizeof(M4_t);
    ub_ci.alignment = 256;
    m_uniform = m_gpu->create_uniform_buffer(&m_allocator, ub_ci);
    m_gpu->bind_resource_to_set(m_uniform, m_resources_set, 0);
    m_world_mat = (M4_t*)m_uniform.uniform_buffer->p;
    *m_world_mat = scale(0.001f, 0.001f, 0.001f);
  }
  {
    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 1024 * 1024;
    vb_ci.alignment = 256;
    vb_ci.stride = 2 * sizeof(float);
    m_vb = m_gpu->create_vertex_buffer(&m_allocator, vb_ci);
    Ttf_loader_t ttf(&temp_allocator);
    ttf.init(g_exe_dir.join(M_txt("assets/UbuntuMono-Regular.ttf")));
    memcpy(m_vb->p, ttf.m_points.m_p, ttf.m_points.len()*sizeof(V2_t));
    m_vertex_count = ttf.m_points.len();
  }
  {
    Input_element_t input_elem = {};
    input_elem.semantic_name = "POSITION";
    input_elem.format = e_format_r32g32_float;
    Input_slot_t input_slot = {};
    input_slot.stride = 2 * sizeof(float);
    input_slot.slot_num = 0;
    input_slot.input_elements = &input_elem;
    input_slot.input_element_count = 1;
    Shader_t* text_vs = m_gpu->compile_shader(&m_allocator, {g_exe_dir.join(M_txt("assets/sample/text_vs"))});
    Shader_t* text_ps = m_gpu->compile_shader(&m_allocator, {g_exe_dir.join(M_txt("assets/sample/text_ps"))});
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = text_vs;
    pso_ci.ps = text_ps;
    pso_ci.input_slot_count = 1;
    pso_ci.input_slots = &input_slot;
    pso_ci.pipeline_layout = m_pipeline_layout;
    pso_ci.render_pass = m_render_pass;
    pso_ci.topology = e_topology_line;
    m_pso = m_gpu->create_pipeline_state_object(&m_allocator, pso_ci);
  }

  return true;
}

void Font_window_t::loop() {
  m_gpu->get_back_buffer();
  m_gpu->cmd_begin();
  m_gpu->cmd_begin_render_pass(m_render_pass);
  m_gpu->cmd_set_pipeline_state(m_pso);
  m_gpu->cmd_set_vertex_buffer(m_vb, 0);
  m_gpu->cmd_set_resource(m_uniform, m_pipeline_layout, m_resources_set, 0);
  m_gpu->cmd_draw(m_vertex_count, 0);
  m_gpu->cmd_end_render_pass(m_render_pass);
  m_gpu->cmd_end();
}

void Font_window_t::destroy() {
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
