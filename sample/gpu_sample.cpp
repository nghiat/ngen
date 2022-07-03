//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/gpu/d3d12/d3d12.h"
#include "core/gpu/vulkan/vulkan.h"
#include "core/loader/obj.h"
#include "core/loader/png.h"
#include "core/log.h"
#include "core/math/float.inl"
#include "core/math/mat4.inl"
#include "core/math/transform.inl"
#include "core/path_utils.h"
#include "core/utils.h"
#include "core/window/window.h"
#include "sample/cam.h"

Command_line_t g_cl;

struct Per_obj_t_ {
  M4_t world;
};

struct Shadow_shared_t_ {
  M4_t light_view;
  M4_t light_proj;
};

struct Final_shared_t_ {
  M4_t view;
  M4_t proj;
  M4_t light_view;
  M4_t light_proj;
  V4_t eye_pos;
  V4_t obj_color;
  V4_t light_pos;
  V4_t light_color;
};

class Gpu_window_t : public Window_t {
public:
  Gpu_window_t(const Os_char* title, int w, int h) : Window_t(title, w, h), m_gpu_allocator("gpu_allocator") {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(E_mouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;

  Linear_allocator_t<> m_gpu_allocator;
  Render_target_t* m_shadow_depth_stencil;
  Render_target_t* m_final_depth_stencil;

  Render_pass_t* m_final_render_pass;
  Render_pass_t* m_shadow_render_pass;
  Render_pass_t* m_texture_render_pass;
  Resources_set_t* m_per_obj_resources_set;
  Resources_set_t* m_shadow_shared_resources_set;
  Resources_set_t* m_final_shared_resources_set;
  Resources_set_t* m_final_shared_samplers;
  Resources_set_t* m_final_shared_srvs;
  Resources_set_t* m_texture_srvs;
  Resources_set_t* m_texture_samplers;
  Pipeline_layout_t* m_shadow_pipeline_layout;
  Pipeline_layout_t* m_final_pipeline_layout;
  Pipeline_layout_t* m_texture_pipeline_layout;
  Pipeline_state_object_t* m_shadow_pso;
  Pipeline_state_object_t* m_final_pso;
  Pipeline_state_object_t* m_texture_pso;

  Uniform_buffer_t* m_shadow_shared_uniform;
  Shadow_shared_t_* m_shadow_shared;
  Uniform_buffer_t* m_final_shared_uniform;
  Final_shared_t_* m_final_shared;
  Uniform_buffer_t* m_per_obj_uniforms[10];
  Per_obj_t_* m_per_obj[10];
  Sampler_t* m_shadow_sampler;
  Sampler_t* m_texture_sampler;
  Image_view_t* m_shadow_depth_stencil_image_view;
  Image_view_t* m_texture_image_view;
  Vertex_buffer_t* m_vertices_vb;
  Vertex_buffer_t* m_normals_vb;
  Vertex_buffer_t* m_texture_v_vb;
  Vertex_buffer_t* m_texture_uv_vb;
  Texture_t* m_texture;

  Gpu_t* m_gpu;
  Cam_t m_cam;
  int m_obj_count;
  int m_obj_vertices_counts[10];

private:
};

bool Gpu_window_t::init() {
  Window_t::init();
  Linear_allocator_t<> temp_allocator("gpu_init_temp_allocator");
  temp_allocator.init();

  m_gpu_allocator.init();
  if (g_cl.get_flag_value("--gpu").get_string().equals("dx12")) {
    m_gpu = g_persistent_allocator->construct<D3d12_t>();
    ((D3d12_t*)m_gpu)->init(this);
  } else {
    m_gpu = g_persistent_allocator->construct<Vulkan_t>();
    ((Vulkan_t*)m_gpu)->init(this);
  }

  // The light is static for now.
  m_cam.init({5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
  Cam_t light_cam;
  light_cam.init({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, this);

  // TODO: ortho?
  M4_t perspective_m4 = perspective(degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);

  {
    m_shadow_depth_stencil = m_gpu->create_depth_stencil(&m_gpu_allocator, { .can_be_sampled = true } );
    m_final_depth_stencil = m_gpu->create_depth_stencil(&m_gpu_allocator, { .can_be_sampled = false } );
  }

  {
    Render_target_description_t shadow_rt_desc = {};
    shadow_rt_desc.render_target = m_shadow_depth_stencil;
    shadow_rt_desc.render_pass_state = e_resource_state_depth_write;
    shadow_rt_desc.state_after = e_resource_state_pixel_shader_resource;

    Render_pass_create_info_t shadow_render_pass_ci = {};
    shadow_render_pass_ci.render_target_count = 1;
    shadow_render_pass_ci.descs = &shadow_rt_desc;
    shadow_render_pass_ci.hint = e_render_pass_hint_shadow;
    shadow_render_pass_ci.should_clear_render_target = true;
    m_shadow_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, shadow_render_pass_ci);
  }
  {
    Render_target_description_t rt_desc = {};
    rt_desc.render_target = m_final_depth_stencil;
    rt_desc.render_pass_state = e_resource_state_depth_write;
    rt_desc.state_after = e_resource_state_depth_write;

    Render_pass_create_info_t final_render_pass_ci = {};
    final_render_pass_ci.render_target_count = 1;
    final_render_pass_ci.descs = &rt_desc;
    final_render_pass_ci.use_swapchain_render_target = true;
    final_render_pass_ci.should_clear_render_target = true;
    m_final_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, final_render_pass_ci);
  }
  {
    Render_pass_create_info_t texture_render_pass_ci = {};
    texture_render_pass_ci.is_last = true;
    texture_render_pass_ci.should_clear_render_target = false;
    m_texture_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, texture_render_pass_ci);
  }
  {
    Resources_set_create_info_t resources_set_ci = {};
    resources_set_ci.binding = 0;
    resources_set_ci.uniform_buffer_count = 1;
    resources_set_ci.visibility = e_shader_stage_vertex;
    m_shadow_shared_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, resources_set_ci);

    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.resources_set = m_shadow_shared_resources_set;
    ub_ci.size = sizeof(Shadow_shared_t_);
    ub_ci.alignment = 256;
    ub_ci.binding = 0;
    m_shadow_shared_uniform = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
    m_shadow_shared = (Shadow_shared_t_*)m_shadow_shared_uniform->p;
    m_shadow_shared->light_view = light_cam.m_view_mat;
    m_shadow_shared->light_proj = perspective_m4;
  }
  {
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.uniform_buffer_count = 1;
      final_resources_set_ci.visibility = (E_shader_stage)(e_shader_stage_vertex | e_shader_stage_fragment);
      m_final_shared_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.sampler_count = 1;
      final_resources_set_ci.visibility = e_shader_stage_fragment;
      m_final_shared_samplers = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.image_count = 1;
      final_resources_set_ci.visibility = e_shader_stage_fragment;
      m_final_shared_srvs = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.resources_set = m_final_shared_resources_set;
    ub_ci.size = sizeof(Final_shared_t_);
    ub_ci.alignment = 256;
    ub_ci.binding = 0;
    m_final_shared_uniform = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
    m_final_shared = (Final_shared_t_*)m_final_shared_uniform->p;
    m_final_shared->view = m_cam.m_view_mat;
    m_final_shared->eye_pos = V3o_v4(m_cam.m_eye, 1.0f);
    m_final_shared->obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
    m_final_shared->light_pos = {10.0f, 10.0f, 10.0f, 1.0f};
    m_final_shared->light_color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_final_shared->proj = perspective_m4;
    m_final_shared->light_view = light_cam.m_view_mat;
    m_final_shared->light_proj = perspective_m4;

    Sampler_create_info_t sampler_ci = {};
    sampler_ci.resources_set = m_final_shared_samplers;
    sampler_ci.binding = 0;
    m_shadow_sampler = m_gpu->create_sampler(&m_gpu_allocator, sampler_ci);

    Image_view_create_info_t image_view_ci = {};
    image_view_ci.resources_set = m_final_shared_srvs;
    image_view_ci.render_target = m_shadow_depth_stencil;
    image_view_ci.binding = 0;
    image_view_ci.format = e_format_r24_unorm_x8_typeless;
    m_shadow_depth_stencil_image_view = m_gpu->create_image_view(&m_gpu_allocator, image_view_ci);
  }
  {
    {
      Resources_set_create_info_t texture_resources_set_ci = {};
      texture_resources_set_ci.binding = 0;
      texture_resources_set_ci.sampler_count = 1;
      texture_resources_set_ci.visibility = e_shader_stage_fragment;
      m_texture_samplers = m_gpu->create_resources_set(&m_gpu_allocator, texture_resources_set_ci);
    }
    {
      Resources_set_create_info_t texture_resources_set_ci = {};
      texture_resources_set_ci.binding = 0;
      texture_resources_set_ci.image_count = 1;
      texture_resources_set_ci.visibility = e_shader_stage_fragment;
      m_texture_srvs = m_gpu->create_resources_set(&m_gpu_allocator, texture_resources_set_ci);
    }

    Sampler_create_info_t sampler_ci = {};
    sampler_ci.resources_set = m_texture_samplers;
    sampler_ci.binding = 0;
    m_texture_sampler = m_gpu->create_sampler(&m_gpu_allocator, sampler_ci);

    {
      Png_loader_t png;
      png.init(&temp_allocator, g_exe_dir.join(M_txt("assets/metal_plates_roughness.png")));
      png.init(&temp_allocator, g_exe_dir.join(M_txt("assets/metal_plates_roughness.png")));
      M_scope_exit(png.destroy());
      Texture_create_info_t ci = {};
      ci.data = png.m_data;
      ci.width = png.m_width;
      ci.height = png.m_height;
      ci.format = e_format_r8_uint;
      m_texture = m_gpu->create_texture(&m_gpu_allocator, ci);
    }

    Image_view_create_info_t image_view_ci = {};
    image_view_ci.resources_set = m_texture_srvs;
    image_view_ci.texture = m_texture;
    image_view_ci.binding = 0;
    image_view_ci.format = e_format_r8_unorm;
    m_texture_image_view = m_gpu->create_image_view(&m_gpu_allocator, image_view_ci);
  }

  {
    Resources_set_create_info_t resources_set_ci = {};
    resources_set_ci.binding = 0;
    resources_set_ci.uniform_buffer_count = 1;
    resources_set_ci.visibility = e_shader_stage_vertex;
    m_per_obj_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, resources_set_ci);
  }

  {
    Resources_set_t* sets[] = {m_per_obj_resources_set, m_shadow_shared_resources_set};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_shadow_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  {
    Resources_set_t* sets[] = {m_per_obj_resources_set, m_final_shared_resources_set, m_final_shared_samplers, m_final_shared_srvs};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_final_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  {
    Resources_set_t* sets[] = {m_texture_srvs, m_texture_samplers};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_texture_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  Input_element_t input_elems[2] = {};
  {
    const Os_char* obj_paths[] = {
        M_txt("assets/wolf.obj"),
        M_txt("assets/plane.obj"),
    };
    Sip vertices_offset = 0;
    Sip normals_offset = 0;
    m_obj_count = static_array_size(obj_paths);

    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 16 * 1024 * 1024;
    vb_ci.alignment = 256;
    vb_ci.stride = sizeof(V4_t);
    m_vertices_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    m_normals_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    {
      for (int i = 0; i < m_obj_count; ++i) {
        Uniform_buffer_create_info_t ub_ci = {};
        ub_ci.resources_set = m_per_obj_resources_set;
        ub_ci.size = sizeof(Per_obj_t_);
        ub_ci.alignment = 256;
        ub_ci.binding = 0;
        m_per_obj_uniforms[i] = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
        m_per_obj[i] = (Per_obj_t_*)m_per_obj_uniforms[i]->p;
        m_per_obj[i]->world = m4_identity();

        Obj_loader_t obj;
        Path_t full_obj_path = g_exe_dir.join(obj_paths[i]);
        obj.init(&temp_allocator, full_obj_path.m_path);
        M_scope_exit(obj.destroy());
        m_obj_vertices_counts[i] = obj.m_vertices.len();
        int vertices_size = m_obj_vertices_counts[i] * sizeof(obj.m_vertices[0]);
        int normals_size = m_obj_vertices_counts[i] * sizeof(obj.m_normals[0]);
        // M_check_return_false(vertices_offset + vertices_size <= m_vertices_subbuffer.bi.range);
        // M_check_return_false(normals_offset + normals_size <= m_normals_subbuffer.bi.range);
        memcpy((U8*)m_vertices_vb->p + vertices_offset, &obj.m_vertices[0], vertices_size);
        memcpy((U8*)m_normals_vb->p + normals_offset, &obj.m_normals[0], normals_size);
        vertices_offset += vertices_size;
        normals_offset += normals_size;
      }
    }

    input_elems[0].semantic_name = "V";
    input_elems[0].format = e_format_r32g32b32a32_float;
    input_elems[0].semantic_index = 0;
    input_elems[0].input_slot = 0;
    input_elems[0].stride = sizeof(V4_t);

    input_elems[1].semantic_name = "N";
    input_elems[1].format = e_format_r32g32b32a32_float;
    input_elems[1].semantic_index = 0;
    input_elems[1].input_slot = 1;
    input_elems[1].stride = sizeof(V4_t);
  }
  Shader_t* shadow_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/sample/shadow_vs"))});
  Shader_t* final_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/sample/shader_vs"))});
  Shader_t* final_ps = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/sample/shader_ps"))});

  {
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = shadow_vs;
    pso_ci.input_element_count = 1;
    pso_ci.input_elements = input_elems;
    pso_ci.pipeline_layout = m_shadow_pipeline_layout;
    pso_ci.render_pass = m_shadow_render_pass;
    pso_ci.enable_depth = true;
    m_shadow_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);
  }
  {
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = final_vs;
    pso_ci.ps = final_ps;
    pso_ci.input_element_count = 2;
    pso_ci.input_elements = input_elems;
    pso_ci.pipeline_layout = m_final_pipeline_layout;
    pso_ci.render_pass = m_final_render_pass;
    pso_ci.enable_depth = true;
    m_final_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);
  }
  {
    Input_element_t texture_input_elems[2] = {};
    texture_input_elems[0].semantic_name = "V";
    texture_input_elems[0].format = e_format_r32g32_float;
    texture_input_elems[0].semantic_index = 0;
    texture_input_elems[0].input_slot = 0;
    texture_input_elems[0].stride = 2 * sizeof(float);

    texture_input_elems[1].semantic_name = "UV";
    texture_input_elems[1].format = e_format_r32g32_float;
    texture_input_elems[1].semantic_index = 0;
    texture_input_elems[1].input_slot = 1;
    texture_input_elems[1].stride = 2 * sizeof(float);
    Pipeline_state_object_create_info_t pso_ci = {};
    Shader_t* texture_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/sample/texture_vs"))});
    Shader_t* texture_ps = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/sample/texture_ps"))});
    pso_ci.vs = texture_vs;
    pso_ci.ps = texture_ps;
    pso_ci.input_element_count = static_array_size(texture_input_elems);
    pso_ci.input_elements = texture_input_elems;
    pso_ci.pipeline_layout = m_texture_pipeline_layout;
    pso_ci.render_pass = m_texture_render_pass;
    m_texture_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);

    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 1024;
    vb_ci.alignment = 256;
    vb_ci.stride = sizeof(V2_t);
    m_texture_v_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    m_texture_uv_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    float vertices[] = {
      -0.5f, -0.5f,
      0.5f, 0.5f,
      -0.5f, 0.5f,

      0.5f, 0.5f,
      -0.5f, -0.5f,
      0.5f, -0.5f,
    };
    memcpy((U8*)m_texture_v_vb->p, vertices, sizeof(vertices));
    float uvs[] = {
      0.0f, 1.0f,
      1.0f, 0.0f,
      0.0f, 0.0f,

      1.0f, 0.0f,
      0.0f, 1.0f,
      1.0f, 1.0f,
    };
    memcpy((U8*)m_texture_uv_vb->p, uvs, sizeof(uvs));
  }

  return true;
}

void Gpu_window_t::destroy() {
}

void Gpu_window_t::loop() {
  m_cam.update();
  m_final_shared->view = m_cam.m_view_mat;
  m_gpu->get_back_buffer();
  m_gpu->cmd_begin();
  {
    m_gpu->cmd_begin_render_pass(m_shadow_render_pass);
    m_gpu->cmd_set_pipeline_state(m_shadow_pso);
    m_gpu->cmd_set_vertex_buffer(m_vertices_vb, 0);
    m_gpu->cmd_set_uniform_buffer(m_shadow_shared_uniform, m_shadow_pipeline_layout, m_shadow_shared_resources_set, 1);
    int vertex_offset = 0;
    for (int i = 0; i < m_obj_count; ++i) {
      m_gpu->cmd_set_uniform_buffer(m_per_obj_uniforms[i], m_shadow_pipeline_layout, m_per_obj_resources_set, 0);
      m_gpu->cmd_draw(m_obj_vertices_counts[i], vertex_offset);
      vertex_offset += m_obj_vertices_counts[i];
    }
    m_gpu->cmd_end_render_pass(m_shadow_render_pass);
  }

  {
    m_gpu->cmd_begin_render_pass(m_final_render_pass);
    m_gpu->cmd_set_pipeline_state(m_final_pso);
    m_gpu->cmd_set_vertex_buffer(m_normals_vb, 1);
    m_gpu->cmd_set_uniform_buffer(m_final_shared_uniform, m_final_pipeline_layout, m_final_shared_resources_set, 1);
    m_gpu->cmd_set_sampler(m_shadow_sampler, m_final_pipeline_layout, m_final_shared_samplers, 2);
    m_gpu->cmd_set_image_view(m_shadow_depth_stencil_image_view, m_final_pipeline_layout, m_final_shared_srvs, 3);
    int vertex_offset = 0;
    for (int i = 0; i < m_obj_count; ++i) {
      m_gpu->cmd_set_uniform_buffer(m_per_obj_uniforms[i], m_final_pipeline_layout, m_per_obj_resources_set, 0);
      m_gpu->cmd_draw(m_obj_vertices_counts[i], vertex_offset);
      vertex_offset += m_obj_vertices_counts[i];
    }
    m_gpu->cmd_end_render_pass(m_final_render_pass);
  }

  {
    m_gpu->cmd_begin_render_pass(m_texture_render_pass);
    m_gpu->cmd_set_pipeline_state(m_texture_pso);
    m_gpu->cmd_set_vertex_buffer(m_texture_v_vb, 0);
    m_gpu->cmd_set_vertex_buffer(m_texture_uv_vb, 1);
    m_gpu->cmd_set_image_view(m_texture_image_view, m_texture_pipeline_layout, m_texture_srvs, 0);
    m_gpu->cmd_set_sampler(m_texture_sampler, m_texture_pipeline_layout, m_texture_samplers, 1);
    m_gpu->cmd_draw(6, 0);
    m_gpu->cmd_end_render_pass(m_texture_render_pass);
  }
  m_gpu->cmd_end();
}

void Gpu_window_t::on_mouse_event(E_mouse mouse, int x, int y, bool is_down) {
  m_cam.mouse_event(mouse, x, y, is_down);
}

void Gpu_window_t::on_mouse_move(int x, int y) {
  m_cam.mouse_move(x, y);
}

int main(int argc, char** argv) {
  core_init(M_txt("gpu_sample.log"));
  g_cl.init(g_persistent_allocator);
  g_cl.register_flag(NULL, "--gpu", e_value_type_string);
  g_cl.parse(argc, argv);
  Gpu_window_t w(M_txt("gpu_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}
