//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/gpu/gpu.h"
#include "core/window/window.h"

#include <d3d12.h>
#include <dxgi1_4.h>

struct D3d12_descriptor_heap_t_ {
  ID3D12DescriptorHeap* heap = NULL;
  U32 increment_size = 0;
  U32 curr_index = 0;
};

struct D3d12_buffer_t_ {
  ID3D12Resource* buffer = NULL;
  void* cpu_p = NULL;
  Sip offset = 0;
};

struct D3d12_descriptor_t_ {
  D3d12_descriptor_heap_t_* descriptor_heap = NULL;
  U32 index = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
};

struct D3d12_render_target_t : Render_target_t {
  ID3D12Resource* resource = NULL;
  D3d12_descriptor_t_ dsv_descriptor;
  D3d12_descriptor_t_ rtv_descriptor;
};

class D3d12_t : public Gpu_t {
public:
  bool init(Window_t* w);
  void destroy() override;
  Texture_t* create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) override;
  Texture_t* create_texture_cube(Allocator_t* allocator, const Texture_cube_create_info_t& ci) override;
  Resources_set_t* create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci) override;
  Pipeline_layout_t* create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci) override;
  Vertex_buffer_t* create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) override;
  Render_target_t* create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) override;
  Render_pass_t* create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) override;
  Resource_t create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) override;
  Resource_t create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) override;
  Resource_t create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) override;
  void bind_resource_to_set(const Resource_t& resource, const Resources_set_t* set, int binding) override;
  Shader_t* compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci) override;
  Pipeline_state_object_t* create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci) override;
  void get_back_buffer() override;
  void cmd_begin() override;
  void cmd_begin_render_pass(Render_pass_t* render_pass) override;
  void cmd_end_render_pass(Render_pass_t* render_pass) override;
  void cmd_set_pipeline_state(Pipeline_state_object_t* pso) override;
  void cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding) override;
  void cmd_set_resource(const Resource_t& resource, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) override;
  void cmd_draw(int vertex_count, int first_vertex) override;
  void cmd_end() override;

  Allocator_t* m_allocator;
  Window_t* m_window = NULL;

  static const int sc_frame_count = 2;
  int m_frame_no;
  ID3D12Device* m_device = NULL;
  ID3D12CommandQueue* m_cmd_queue = NULL;
  IDXGISwapChain3* m_swap_chain = NULL;
  D3d12_render_target_t m_swapchain_rts[sc_frame_count];
  ID3D12CommandAllocator* m_cmd_allocators[sc_frame_count];
  ID3D12GraphicsCommandList* m_cmd_list;

  D3d12_descriptor_heap_t_ m_rtv_heap;
  D3d12_descriptor_heap_t_ m_cbv_srv_heap;
  D3d12_descriptor_heap_t_ m_dsv_heap;
  D3d12_descriptor_heap_t_ m_sampler_heap;

  D3d12_buffer_t_ m_uniform_buffer;
  D3d12_buffer_t_ m_vertex_buffer;
  D3d12_buffer_t_ m_upload_buffer;
  ID3D12Fence* m_fence;
  HANDLE m_fence_event;
  U64 m_fence_vals[sc_frame_count] = {};
private:
  void create_descriptor_heap_(D3d12_descriptor_heap_t_* dh, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, U32 max_descriptor_count);
  void wait_for_gpu_();
};
