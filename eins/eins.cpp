//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/dynamic_array.h"
#include "core/fixed_array.h"
#include "core/linear_allocator.h"
#include "core/loader/dae.h"
#include "core/loader/dds.h"
#include "core/loader/obj.h"
#include "core/loader/png.h"
#include "core/log.h"
#include "core/math/float.h"
#include "core/math/mat4.h"
#include "core/math/transform.h"
#include "core/math/vec3.h"
#include "core/mono_time.h"
#include "core/path_utils.h"
#include "core/string.h"
#include "core/utils.h"
#include "core/window/window.h"
#include "eins/cam.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

Command_line_t g_cl(g_persistent_allocator);

struct Per_obj_t_ {
  M4_t world = {};
  M4_t joints[300] = {};
  M4_t inv_bind_mat[300] = {};
};

struct Shared_t_ {
  M4_t view;
  M4_t proj;
  M4_t light_view;
  M4_t light_proj;
  V4_t eye_pos;
  V4_t obj_color;
  V4_t light_pos;
  V4_t light_color;
};

static V3_t spherical_to_cartesian(float r, float phi, float theta) {
  V3_t v;
  v.y = r * cos(theta);
  float temp = r * sin(theta);
  v.x = temp * cos(phi);
  v.z = temp * sin(phi);
  return v;
}

struct Textured_sphere_t {
  Textured_sphere_t(Allocator_t* allocator) : v(allocator), uv(allocator) {}
  Dynamic_array_t<V3_t> v; // normal are the same as v
  Dynamic_array_t<V2_t> uv;
};

static Textured_sphere_t generate_sphere(Allocator_t* allocator, float r, int n) {
  int vertex_count = n * ((n-2)*6 + 2*3);
  Textured_sphere_t s(allocator);
  s.v.reserve(vertex_count);
  s.uv.reserve(vertex_count);
  for (int i = 0; i < n; ++i) {
    float phi = 2 * M_pi_f * i/ n;
    float phi2 = 2 * M_pi_f * (i + 1)/ n;
    for (int j = 0; j < n; ++j) {
      float theta = M_pi_f * j / n;
      float theta2 = M_pi_f * (j + 1) / n;
      V3_t v1 = spherical_to_cartesian(r, phi, theta);
      V3_t v2 = spherical_to_cartesian(r, phi2, theta);
      V3_t v3 = spherical_to_cartesian(r, phi, theta2);
      V3_t v4 = spherical_to_cartesian(r, phi2, theta2);
      V2_t uv1 = { i * 1.0f / n, j * 1.0f / n };
      V2_t uv2 = { (i +1) * 1.0f / n, j * 1.0f / n };
      V2_t uv3 = { i * 1.0f / n, (j + 1) * 1.0f / n };
      V2_t uv4 = { (i + 1) * 1.0f / n, (j + 1) * 1.0f / n };
      if (j == 0) {
        s.v.append(v1);
        s.v.append(v4);
        s.v.append(v3);

        s.uv.append(uv1);
        s.uv.append(uv4);
        s.uv.append(uv3);
      } else if (j == n - 1) {
        s.v.append(v2);
        s.v.append(v3);
        s.v.append(v1);

        s.uv.append(uv2);
        s.uv.append(uv3);
        s.uv.append(uv1);
      } else {
        s.v.append(v1);
        s.v.append(v2);
        s.v.append(v3);
        s.uv.append(uv1);
        s.uv.append(uv2);
        s.uv.append(uv3);

        s.v.append(v2);
        s.v.append(v4);
        s.v.append(v3);
        s.uv.append(uv2);
        s.uv.append(uv4);
        s.uv.append(uv3);
      }
    }
  }
  return s;
}

class Eins_window_t : public Window_t {
public:
  Eins_window_t(const Os_char* title, int w, int h) : Window_t(title, w, h), m_gpu_allocator("gpu_allocator"), m_dae_model(&m_gpu_allocator) {}

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
  Render_pass_t* m_cube_render_pass;
  Render_pass_t* m_pbr_render_pass;
  Resources_set_t* m_per_obj_resources_set;
  Resources_set_t* m_per_obj_cube_resources_set;
  Resources_set_t* m_shadow_shared_resources_set;
  Resources_set_t* m_shared_resources_set;
  Resources_set_t* m_shared_samplers;
  Resources_set_t* m_shared_srvs;
  Resources_set_t* m_cube_srvs;
  Resources_set_t* m_pbr_samplers;
  Resources_set_t* m_pbr_srvs;
  Pipeline_layout_t* m_shadow_pipeline_layout;
  Pipeline_layout_t* m_final_pipeline_layout;
  Pipeline_layout_t* m_cube_pipeline_layout;
  Pipeline_layout_t* m_pbr_pipeline_layout;
  Pipeline_state_object_t* m_shadow_pso;
  Pipeline_state_object_t* m_final_pso;
  Pipeline_state_object_t* m_cube_pso;
  Pipeline_state_object_t* m_pbr_pso;

  Resource_t m_shared_uniform;
  Shared_t_* m_shared;
  Resource_t m_per_obj_uniforms[10];
  Resource_t m_per_obj_pbr_uniform;
  Resource_t m_per_obj_cube_uniform;
  Per_obj_t_* m_per_obj[10];
  Per_obj_t_* m_per_obj_pbr;
  Per_obj_t_* m_per_obj_cube;
  Resource_t m_sampler;
  Resource_t m_shadow_depth_stencil_image_view;
  Index_buffer_t* m_index_buffer;
  Vertex_buffer_t* m_vertices_vb;
  Vertex_buffer_t* m_normals_vb;
  Vertex_buffer_t* m_pbr_v_vb;
  Vertex_buffer_t* m_pbr_uv_vb;
  Vertex_buffer_t* m_cube_v_vb;

  Texture_t* m_albedo_texture;
  Resource_t m_albedo_srv;
  Texture_t* m_normal_texture;
  Resource_t m_normal_srv;
  Texture_t* m_metallic_texture;
  Resource_t m_metallic_srv;
  Texture_t* m_roughness_texture;
  Resource_t m_roughness_srv;
  Texture_t* m_cube_texture;
  Resource_t m_cube_srv;

  Gpu_t* m_gpu;
  Cam_t m_cam;
  int m_obj_count;
  int m_obj_vertices_counts[10];
  int m_obj_indices_counts[10];
  int m_sphere_vertice_count;
  S64 m_time_start;

  Dae_loader_t m_dae_model;
private:
  void create_texture_and_srv_(Texture_t** texture, Resource_t* srv, const Path_t& path, Resources_set_t* set, int binding, E_format srv_format);
};

bool Eins_window_t::init() {
  Window_t::init();
  Linear_allocator_t<> temp_allocator("gpu_init_temp_allocator");
  M_scope_exit(temp_allocator.destroy());

  m_gpu = Gpu_t::init(g_persistent_allocator, &g_cl, this);

  // The light is static for now.
  m_cam.init({200.0f, 200.0f, 200.0f}, {0.0f, 0.0f, 0.0f}, this);
  Cam_t light_cam;
  light_cam.init({200.0f, 200.0f, 200.0f}, {0.0f, 0.0f, 0.0f}, this);

  // TODO: ortho?
  M4_t perspective_m4 = perspective(degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 500.0f);

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
    final_render_pass_ci.is_last = true;
    m_final_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, final_render_pass_ci);
  }
  {
    Render_pass_create_info_t cube_render_pass_ci = {};
    cube_render_pass_ci.use_swapchain_render_target = true;
    m_cube_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, cube_render_pass_ci);
  }
  {
    Render_pass_create_info_t render_pass_ci = {};
    render_pass_ci.is_last = true;
    render_pass_ci.should_clear_render_target = false;
    m_pbr_render_pass = m_gpu->create_render_pass(&m_gpu_allocator, render_pass_ci);
  }
  {
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.uniform_buffer_count = 1;
      final_resources_set_ci.visibility = (E_shader_stage)(e_shader_stage_vertex | e_shader_stage_fragment);
      m_shared_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.sampler_count = 1;
      final_resources_set_ci.visibility = e_shader_stage_fragment;
      m_shared_samplers = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    {
      Resources_set_create_info_t final_resources_set_ci = {};
      final_resources_set_ci.binding = 0;
      final_resources_set_ci.image_count = 1;
      final_resources_set_ci.visibility = e_shader_stage_fragment;
      m_shared_srvs = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
      m_cube_srvs = m_gpu->create_resources_set(&m_gpu_allocator, final_resources_set_ci);
    }
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.size = sizeof(Shared_t_);
    ub_ci.alignment = 256;
    m_shared_uniform = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
    m_gpu->bind_resource_to_set(m_shared_uniform, m_shared_resources_set, 0);
    m_shared = (Shared_t_*)m_shared_uniform.uniform_buffer->p;
    m_shared->view = m_cam.m_view_mat;
    m_shared->eye_pos = V3o_v4(m_cam.m_eye, 1.0f);
    m_shared->obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
    m_shared->light_pos = {200.0f, 200.0f, 200.0f, 1.0f};
    m_shared->light_color = {1.0f, 1.0f, 1.0f, 1.0f};
    m_shared->proj = perspective_m4;
    m_shared->light_view = light_cam.m_view_mat;
    m_shared->light_proj = perspective_m4;

    Sampler_create_info_t sampler_ci = {};
    m_sampler = m_gpu->create_sampler(&m_gpu_allocator, sampler_ci);
    m_gpu->bind_resource_to_set(m_sampler, m_shared_samplers, 0);

    Image_view_create_info_t image_view_ci = {};
    image_view_ci.render_target = m_shadow_depth_stencil;
    image_view_ci.format = e_format_r24_unorm_x8_typeless;
    m_shadow_depth_stencil_image_view = m_gpu->create_image_view(&m_gpu_allocator, image_view_ci);
    m_gpu->bind_resource_to_set(m_shadow_depth_stencil_image_view, m_shared_srvs, 0);
  }
  {
    {
      Resources_set_create_info_t ci = {};
      ci.binding = 0;
      ci.sampler_count = 1;
      ci.visibility = e_shader_stage_fragment;
      m_pbr_samplers = m_gpu->create_resources_set(&m_gpu_allocator, ci);
      m_gpu->bind_resource_to_set(m_sampler, m_pbr_samplers, 0);
    }
    {
      Resources_set_create_info_t ci = {};
      ci.binding = 0;
      ci.image_count = 4;
      ci.visibility = e_shader_stage_fragment;
      m_pbr_srvs = m_gpu->create_resources_set(&m_gpu_allocator, ci);
    }
    create_texture_and_srv_(&m_albedo_texture, &m_albedo_srv, g_exe_dir.join(M_txt("assets/basecolor.dds")), m_pbr_srvs, 0, e_format_bc7_unorm);
    create_texture_and_srv_(&m_normal_texture, &m_normal_srv, g_exe_dir.join(M_txt("assets/normal.dds")), m_pbr_srvs, 1, e_format_bc7_unorm);
    create_texture_and_srv_(&m_metallic_texture, &m_metallic_srv, g_exe_dir.join(M_txt("assets/metallic.dds")), m_pbr_srvs, 2, e_format_bc7_unorm);
    create_texture_and_srv_(&m_roughness_texture, &m_roughness_srv, g_exe_dir.join(M_txt("assets/roughness.dds")), m_pbr_srvs, 3, e_format_bc7_unorm);
    {
      Scope_allocator_t<> scope_allocator(&temp_allocator);
      Path_t paths[6] = {
        g_exe_dir.join(M_txt("assets/posx.dds")),
        g_exe_dir.join(M_txt("assets/negx.dds")),
        g_exe_dir.join(M_txt("assets/posy.dds")),
        g_exe_dir.join(M_txt("assets/negy.dds")),
        g_exe_dir.join(M_txt("assets/posz.dds")),
        g_exe_dir.join(M_txt("assets/negz.dds")),
      };
      U8* cube_data = NULL;
      U32 dimension = 0;
      U32 one_face_size = 0;
      E_format format;
      Texture_create_info_t ci = {};
      for (int i = 0; i < 6; ++i) {
        Linear_allocator_t<> texture_temp_allocator("texture_temp_allocator");
        M_scope_exit(texture_temp_allocator.destroy());
        Dds_loader_t cube_texture(&texture_temp_allocator);
        cube_texture.init(paths[i]);
        ci = get_texture_create_info(cube_texture);
        M_check(cube_texture.m_header->width == cube_texture.m_header->height);
        if (dimension == 0) {
          dimension = cube_texture.m_header->width;
          one_face_size = ci.row_count * ci.row_pitch;
          cube_data = (U8*)scope_allocator.alloc(6 * one_face_size);
          format = cube_texture.m_format;
        } else {
          M_check(dimension == cube_texture.m_header->width);
          M_check(format == cube_texture.m_format);
        }
        memcpy(cube_data + i*one_face_size, cube_texture.m_data, one_face_size);
      }
      ci.data = cube_data;
      m_cube_texture = m_gpu->create_texture_cube(&m_gpu_allocator, ci);
      Image_view_create_info_t cube_srv_ci = {};
      cube_srv_ci.texture = m_cube_texture;
      cube_srv_ci.format = e_format_bc7_unorm;
      m_cube_srv = m_gpu->create_image_view(&m_gpu_allocator, cube_srv_ci);
      m_gpu->bind_resource_to_set(m_cube_srv, m_cube_srvs, 0);
    }
  }

  {
    Resources_set_create_info_t resources_set_ci = {};
    resources_set_ci.binding = 0;
    resources_set_ci.uniform_buffer_count = 1;
    resources_set_ci.visibility = e_shader_stage_vertex;
    m_per_obj_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, resources_set_ci);
    m_per_obj_cube_resources_set = m_gpu->create_resources_set(&m_gpu_allocator, resources_set_ci);
  }

  {
    Resources_set_t* sets[] = {m_per_obj_resources_set, m_shared_resources_set};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_shadow_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  {
    Resources_set_t* sets[] = {m_per_obj_resources_set, m_shared_resources_set, m_shared_samplers, m_shared_srvs};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_final_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }
  {
    Resources_set_t* sets[] = {m_per_obj_cube_resources_set, m_shared_resources_set, m_shared_samplers, m_cube_srvs};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_cube_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  {
    Resources_set_t* sets[] = {m_per_obj_resources_set, m_shared_resources_set, m_pbr_samplers, m_pbr_srvs};
    Pipeline_layout_create_info_t ci = {};
    ci.set_count = static_array_size(sets);
    ci.sets = sets;
    m_pbr_pipeline_layout = m_gpu->create_pipeline_layout(&m_gpu_allocator, ci);
  }

  {
    // const Os_char* obj_paths[] = {
    //     // M_txt("assets/wolf.obj"),
    //     M_txt("assets/plane.obj"),
    // };
    const Os_char* dae_paths[] = {
        M_txt("assets/pirate.dae"),
    };
    Sip vertices_offset = 0;
    Sip normals_offset = 0;
    m_obj_count = static_array_size(dae_paths);

    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 32 * 1024 * 1024;
    vb_ci.alignment = 256;
    vb_ci.stride = sizeof(Vertex_t);
    m_vertices_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    m_normals_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    Index_buffer_create_info_t ib_ci = {};
    ib_ci.size = 16 * 1024 * 1024;
    m_index_buffer = m_gpu->create_index_buffer(&m_gpu_allocator, ib_ci);
    {
      for (int i = 0; i < m_obj_count; ++i) {
        Uniform_buffer_create_info_t ub_ci = {};
        ub_ci.size = sizeof(Per_obj_t_);
        ub_ci.alignment = 256;
        m_per_obj_uniforms[i] = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
        m_gpu->bind_resource_to_set(m_per_obj_uniforms[i], m_per_obj_resources_set, 0);
        m_per_obj[i] = (Per_obj_t_*)m_per_obj_uniforms[i].uniform_buffer->p;

        Path_t full_obj_path = g_exe_dir.join(dae_paths[i]);
        m_dae_model.init(full_obj_path);
        memset(m_per_obj[i], 0, sizeof(Per_obj_t_));
        m_per_obj[i]->world = m4_identity();
        m_dae_model.update_joint_matrices_at(0.f);
        memcpy(m_per_obj[i]->joints, m_dae_model.m_joint_matrices.m_p, m_dae_model.m_joint_matrices.len() * sizeof(M4_t));
        memcpy(m_per_obj[i]->inv_bind_mat, m_dae_model.m_inv_bind_matrices.m_p, m_dae_model.m_inv_bind_matrices.len() * sizeof(M4_t));
        m_obj_vertices_counts[i] = m_dae_model.m_vertices.len();
        int vertices_size = m_obj_vertices_counts[i] * sizeof(m_dae_model.m_vertices[0]);
        // int normals_size = m_obj_vertices_counts[i] * sizeof(obj.m_normals[0]);
        // M_check_return_false(vertices_offset + vertices_size <= m_vertices_subbuffer.bi.range);
        // M_check_return_false(normals_offset + normals_size <= m_normals_subbuffer.bi.range);
        memcpy((U8*)m_vertices_vb->p + vertices_offset, &m_dae_model.m_vertices[0], vertices_size);
        // memcpy((U8*)m_normals_vb->p + normals_offset, &obj.m_normals[0], normals_size);
        vertices_offset += vertices_size;
        // normals_offset += normals_size;
      }
    }
  }
  Fixed_array_t<Input_element_t, 16> input_elems;
  {
    Input_element_t input_elem = {};
    input_elem.semantic_name = "POSITION";
    input_elem.format = e_format_r32g32b32a32_float;
    input_elems.append(input_elem);
  }
  {
    Input_element_t input_elem = {};
    input_elem.semantic_name = "NORMAL";
    input_elem.format = e_format_r32g32b32_float;
    input_elems.append(input_elem);
  }
  {
    Input_element_t input_elem = {};
    input_elem.semantic_name = "BLENDINDICES";
    input_elem.format = e_format_r32g32b32a32_uint;
    input_elem.matrix_row_count = 4;
    input_elems.append(input_elem);
  }
  {
    Input_element_t input_elem = {};
    input_elem.semantic_name = "BLENDWEIGHT";
    input_elem.format = e_format_r32g32b32a32_float;
    input_elem.matrix_row_count = 4;
    input_elems.append(input_elem);
  }
  Input_slot_t input_slot = {};
  input_slot.stride = sizeof(Vertex_t);
  input_slot.slot_num = 0;
  input_slot.input_elements = input_elems.m_p;

  Shader_t* shadow_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/shadow_vs"))});
  Shader_t* final_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/shader_vs"))});
  Shader_t* final_ps = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/shader_ps"))});

  {
    input_slot.input_element_count = 1;
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = shadow_vs;
    pso_ci.input_slot_count = 1;
    pso_ci.input_slots = &input_slot;
    pso_ci.pipeline_layout = m_shadow_pipeline_layout;
    pso_ci.render_pass = m_shadow_render_pass;
    pso_ci.enable_depth = true;
    m_shadow_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);
  }
  {
    input_slot.input_element_count = input_elems.len();
    Pipeline_state_object_create_info_t pso_ci = {};
    pso_ci.vs = final_vs;
    pso_ci.ps = final_ps;
    pso_ci.input_slot_count = 1;
    pso_ci.input_slots = &input_slot;
    pso_ci.pipeline_layout = m_final_pipeline_layout;
    pso_ci.render_pass = m_final_render_pass;
    pso_ci.enable_depth = true;
    m_final_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);
  }
  {
    Input_element_t cube_input_elem = {};
    cube_input_elem.semantic_name = "V";
    cube_input_elem.format = e_format_r32g32b32_float;
    Input_slot_t cube_input_slot = {};
    cube_input_slot.stride = sizeof(V3_t);
    cube_input_slot.slot_num = 0;
    cube_input_slot.input_element_count = 1;
    cube_input_slot.input_elements = &cube_input_elem;

    Pipeline_state_object_create_info_t pso_ci = {};
    Shader_t* cube_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/cube_vs"))});
    Shader_t* cube_ps = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/cube_ps"))});
    pso_ci.vs = cube_vs;
    pso_ci.ps = cube_ps;
    pso_ci.input_slot_count = 1;
    pso_ci.input_slots = &cube_input_slot;
    pso_ci.pipeline_layout = m_cube_pipeline_layout;
    pso_ci.render_pass = m_cube_render_pass;
    m_cube_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);

    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = sizeof(V3_t) * 6 * 6;
    vb_ci.alignment = 256;
    vb_ci.stride = sizeof(V3_t);
    m_cube_v_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    V3_t* cube_v = (V3_t*)m_cube_v_vb->p;
    // Neg x
    cube_v[0] = V3_t{-1.0f, -1.0f, -1.0f};
    cube_v[1] = V3_t{-1.0f, 1.0f, 1.0f};
    cube_v[2] = V3_t{-1.0f, -1.0f, 1.0f};
    cube_v[3] = V3_t{-1.0f, -1.0f, -1.0f};
    cube_v[4] = V3_t{-1.0f, 1.0f, -1.0f};
    cube_v[5] = V3_t{-1.0f, 1.0f, 1.0f};

    // Neg y
    cube_v[6] = V3_t{-1.0f, -1.0f, -1.0f};
    cube_v[7] = V3_t{-1.0f, -1.0f, 1.0f};
    cube_v[8] = V3_t{1.0f, -1.0f, -1.0f};
    cube_v[9] = V3_t{-1.0f, -1.0f, 1.0f};
    cube_v[10] = V3_t{1.0f, -1.0f, 1.0f};
    cube_v[11] = V3_t{1.0f, -1.0f, -1.0f};

    // Neg z
    cube_v[12] = V3_t{-1.0f, -1.0f, -1.0f};
    cube_v[13] = V3_t{1.0f, -1.0f, -1.0f};
    cube_v[14] = V3_t{-1.0f, 1.0f, -1.0f};
    cube_v[15] = V3_t{-1.0f, 1.0f, -1.0f};
    cube_v[16] = V3_t{1.0f, -1.0f, -1.0f};
    cube_v[17] = V3_t{1.0f, 1.0f, -1.0f};

    // Pos x
    cube_v[18] = V3_t{1.0f, 1.0f, 1.0f};
    cube_v[19] = V3_t{1.0f, 1.0f, -1.0f};
    cube_v[20] = V3_t{1.0f, -1.0f, 1.0f};
    cube_v[21] = V3_t{1.0f, 1.0f, -1.0f};
    cube_v[22] = V3_t{1.0f, -1.0f, -1.0f};
    cube_v[23] = V3_t{1.0f, -1.0f, 1.0f};

    // Pos y
    cube_v[24] = V3_t{-1.0f, 1.0f, -1.0f};
    cube_v[25] = V3_t{1.0f, 1.0f, -1.0f};
    cube_v[26] = V3_t{-1.0f, 1.0f, 1.0f};
    cube_v[27] = V3_t{1.0f, 1.0f, -1.0f};
    cube_v[28] = V3_t{1.0f, 1.0f, 1.0f};
    cube_v[29] = V3_t{-1.0f, 1.0f, 1.0f};

    // Pos z
    cube_v[30] = V3_t{-1.0f, -1.0f, 1.0f};
    cube_v[31] = V3_t{-1.0f, 1.0f, 1.0f};
    cube_v[32] = V3_t{1.0f, 1.0f, 1.0f};
    cube_v[33] = V3_t{1.0f, 1.0f, 1.0f};
    cube_v[34] = V3_t{1.0f, -1.0f, 1.0f};
    cube_v[35] = V3_t{-1.0f, -1.0f, 1.0f};
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.size = sizeof(Per_obj_t_);
    ub_ci.alignment = 256;
    m_per_obj_cube_uniform = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
    m_gpu->bind_resource_to_set(m_per_obj_cube_uniform, m_per_obj_cube_resources_set, 0);
    m_per_obj_cube = (Per_obj_t_*)m_per_obj_cube_uniform.uniform_buffer->p;
    m_per_obj_cube->world = scale(1000.f, 1000.f, 1000.f);
  }
  {
    Input_element_t pbr_input_elems[3] = {};
    Input_slot_t pbr_input_slots[3] = {};
    pbr_input_elems[0].semantic_name = "V";
    pbr_input_elems[0].format = e_format_r32g32b32_float;
    pbr_input_slots[0].input_element_count = 1;
    pbr_input_slots[0].input_elements = &pbr_input_elems[0];
    pbr_input_slots[0].slot_num = 0;
    pbr_input_slots[0].stride = sizeof(V3_t);

    pbr_input_elems[1].semantic_name = "UV";
    pbr_input_elems[1].format = e_format_r32g32_float;
    pbr_input_slots[1].input_element_count = 1;
    pbr_input_slots[1].input_elements = &pbr_input_elems[1];
    pbr_input_slots[1].slot_num = 1;
    pbr_input_slots[1].stride = sizeof(V2_t);

    pbr_input_elems[2].semantic_name = "NORMAL";
    pbr_input_elems[2].format = e_format_r32g32b32_float;
    pbr_input_slots[2].input_element_count = 1;
    pbr_input_slots[2].input_elements = &pbr_input_elems[2];
    pbr_input_slots[2].slot_num = 2;
    pbr_input_slots[2].stride = sizeof(V3_t);

    Pipeline_state_object_create_info_t pso_ci = {};
    Shader_t* pbr_vs = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/pbr_vs"))});
    Shader_t* pbr_ps = m_gpu->compile_shader(&m_gpu_allocator, {g_exe_dir.join(M_txt("assets/eins/pbr_ps"))});
    pso_ci.vs = pbr_vs;
    pso_ci.ps = pbr_ps;
    pso_ci.input_slot_count = 3;
    pso_ci.input_slots = pbr_input_slots;
    pso_ci.pipeline_layout = m_pbr_pipeline_layout;
    pso_ci.render_pass = m_pbr_render_pass;
    m_pbr_pso = m_gpu->create_pipeline_state_object(&m_gpu_allocator, pso_ci);

    Scope_allocator_t<> scope_allocator(&temp_allocator);
    auto sphere = generate_sphere(&scope_allocator, 5, 20);
    Vertex_buffer_create_info_t vb_ci = {};
    vb_ci.size = 1024*1024;
    vb_ci.alignment = 256;
    vb_ci.stride = sizeof(V3_t);
    m_pbr_v_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    memcpy(m_pbr_v_vb->p, sphere.v.m_p, sphere.v.len() * sizeof(V3_t));
    vb_ci.stride = sizeof(V2_t);
    m_pbr_uv_vb = m_gpu->create_vertex_buffer(&m_gpu_allocator, vb_ci);
    memcpy(m_pbr_uv_vb->p, sphere.uv.m_p, sphere.v.len() * sizeof(V2_t));
    m_sphere_vertice_count = sphere.v.len();
    Uniform_buffer_create_info_t ub_ci = {};
    ub_ci.size = sizeof(Per_obj_t_);
    ub_ci.alignment = 256;
    m_per_obj_pbr_uniform = m_gpu->create_uniform_buffer(&m_gpu_allocator, ub_ci);
    // m_gpu->bind_resource_to_set(m_per_obj_pbr_uniform, m_per_obj_resources_set, 0);
    m_per_obj_pbr = (Per_obj_t_*)m_per_obj_pbr_uniform.uniform_buffer->p;
    m_per_obj_pbr->world = m4_identity();
  }

  m_time_start = mono_time_now();
  return true;
}

void Eins_window_t::destroy() {
}

void Eins_window_t::loop() {
  m_cam.update();
  m_shared->view = m_cam.m_view_mat;
  F64 delta_s = mono_time_to_s(mono_time_now() - m_time_start);

  m_dae_model.update_joint_matrices_at(delta_s);
  memcpy(m_per_obj[0]->joints, m_dae_model.m_joint_matrices.m_p, m_dae_model.m_joint_matrices.len() * sizeof(M4_t));

  m_gpu->get_back_buffer();
  m_gpu->cmd_begin();
  m_gpu->cmd_set_viewport();
  m_gpu->cmd_set_scissor();
  {
    m_gpu->cmd_begin_render_pass(m_shadow_render_pass);
    m_gpu->cmd_set_pipeline_state(m_shadow_pso);
    m_gpu->cmd_set_vertex_buffer(m_vertices_vb, 0);
    m_gpu->cmd_set_index_buffer(m_index_buffer);
    m_gpu->cmd_set_resource(m_shared_uniform, m_shadow_pipeline_layout, m_shared_resources_set, 1);
    int vertex_offset = 0;
    for (int i = 0; i < m_obj_count; ++i) {
      m_gpu->cmd_set_resource(m_per_obj_uniforms[i], m_shadow_pipeline_layout, m_per_obj_resources_set, 0);
      // m_gpu->cmd_draw_index(m_obj_indices_counts[i], 1, 0, 0, 0);
      m_gpu->cmd_draw(m_obj_vertices_counts[i], 0);
      vertex_offset += m_obj_vertices_counts[i];
    }
    m_gpu->cmd_end_render_pass(m_shadow_render_pass);
  }

  {
    m_gpu->cmd_begin_render_pass(m_final_render_pass);
    m_gpu->cmd_set_pipeline_state(m_final_pso);
    m_gpu->cmd_set_vertex_buffer(m_normals_vb, 1);
    m_gpu->cmd_set_resource(m_shared_uniform, m_final_pipeline_layout, m_shared_resources_set, 1);
    m_gpu->cmd_set_resource(m_sampler, m_final_pipeline_layout, m_shared_samplers, 2);
    m_gpu->cmd_set_resource(m_shadow_depth_stencil_image_view, m_final_pipeline_layout, m_shared_srvs, 3);
    int vertex_offset = 0;
    for (int i = 0; i < m_obj_count; ++i) {
      m_gpu->cmd_set_resource(m_per_obj_uniforms[i], m_final_pipeline_layout, m_per_obj_resources_set, 0);
      // m_gpu->cmd_draw_index(m_obj_indices_counts[i], 1, 0, 0, 0);
      m_gpu->cmd_draw(m_obj_vertices_counts[i], 0);
      vertex_offset += m_obj_vertices_counts[i];
    }
    m_gpu->cmd_end_render_pass(m_final_render_pass);
  }

  // {
  //   m_gpu->cmd_begin_render_pass(m_cube_render_pass);
  //   m_gpu->cmd_set_pipeline_state(m_cube_pso);
  //   m_gpu->cmd_set_vertex_buffer(m_cube_v_vb, 0);
  //   m_gpu->cmd_set_resource(m_shared_uniform, m_cube_pipeline_layout, m_shared_resources_set, 1);
  //   m_gpu->cmd_set_resource(m_per_obj_cube_uniform, m_cube_pipeline_layout, m_per_obj_cube_resources_set, 0);
  //   m_gpu->cmd_set_resource(m_sampler, m_cube_pipeline_layout, m_shared_samplers, 2);
  //   m_gpu->cmd_set_resource(m_cube_srv, m_cube_pipeline_layout, m_cube_srvs, 3);
  //   m_gpu->cmd_draw(6 * 6, 0);
  //   m_gpu->cmd_end_render_pass(m_cube_render_pass);
  // }

  // {
  //   m_gpu->cmd_begin_render_pass(m_pbr_render_pass);
  //   m_gpu->cmd_set_pipeline_state(m_pbr_pso);
  //   m_gpu->cmd_set_vertex_buffer(m_pbr_v_vb, 0);
  //   m_gpu->cmd_set_vertex_buffer(m_pbr_uv_vb, 1);
  //   m_gpu->cmd_set_vertex_buffer(m_pbr_v_vb, 2);
  //   m_gpu->cmd_set_resource(m_shared_uniform, m_pbr_pipeline_layout, m_shared_resources_set, 1);
  //   m_gpu->cmd_set_resource(m_per_obj_pbr_uniform, m_pbr_pipeline_layout, m_per_obj_resources_set, 0);
  //   m_gpu->cmd_set_resource(m_sampler, m_pbr_pipeline_layout, m_pbr_samplers, 2);
  //   m_gpu->cmd_set_resource(m_albedo_srv, m_pbr_pipeline_layout, m_pbr_srvs, 3);
  //   m_gpu->cmd_draw(m_sphere_vertice_count, 0);
  //   m_gpu->cmd_end_render_pass(m_pbr_render_pass);
  // }
  m_gpu->cmd_end();
}

void Eins_window_t::on_mouse_event(E_mouse mouse, int x, int y, bool is_down) {
  m_cam.mouse_event(mouse, x, y, is_down);
}

void Eins_window_t::on_mouse_move(int x, int y) {
  m_cam.mouse_move(x, y);
}

void Eins_window_t::create_texture_and_srv_(Texture_t** texture, Resource_t* srv, const Path_t& path, Resources_set_t* set, int binding, E_format srv_format) {
  Linear_allocator_t<> temp_allocator("texture_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  Dds_loader_t dds(&temp_allocator);
  dds.init(path);
  *texture = m_gpu->create_texture(&m_gpu_allocator, get_texture_create_info(dds));

  Image_view_create_info_t image_view_ci = {};
  image_view_ci.texture = *texture;
  image_view_ci.format = srv_format;
  *srv = m_gpu->create_image_view(&m_gpu_allocator, image_view_ci);
  m_gpu->bind_resource_to_set(*srv, set, binding);
}

int main(int argc, char** argv) {
  core_init(M_txt("eins.log"));
  g_cl.register_flag(NULL, "--gpu", e_value_type_string);
  g_cl.parse(argc, argv);
  Eins_window_t w(M_txt("eins"), 1024, 768);
  w.init();
  w.os_loop();
  core_destroy();
  return 0;
}
