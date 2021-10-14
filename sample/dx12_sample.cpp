//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/allocator.h"
#include "core/compiler.h"
#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/linear_allocator.h"
#include "core/loader/obj.h"
#include "core/log.h"
#include "core/math/float.inl"
#include "core/math/mat4.inl"
#include "core/math/transform.inl"
#include "core/math/vec2.inl"
#include "core/mono_time.h"
#include "core/path_utils.h"
#include "core/utils.h"
#include "core/window/window.h"
#include "core/window/window.h"
#include "sample/cam.h"
#include "sample/quake_console.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <third_party/stb/stb_truetype.h>

#include <ctype.h>
#include <math.h>
#include <wchar.h>

#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi1_4.h>

#define DX_CHECK_RETURN(condition) CHECK_RETURN(condition == S_OK)
#define DX_CHECK_RETURN_FALSE(condition) CHECK_RETURN_VAL(condition == S_OK, false)

#if IS_CLANG()
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

struct codepoint_t {
  V2 uv_top_left;
  V2 uv_bottom_right;
  int x0, x1, y0, y1;
  int advance;
  int lsb;
};

struct font_t {
  stbtt_fontinfo fontinfo;
  codepoint_t codepoints[32*1024];
  int baseline;
  int line_space;
  float scale;
} g_font;

struct per_obj_cb_t {
  M4 world;
};

struct shadow_shared_cb_t {
  M4 light_view;
  M4 light_proj;
};

struct final_shared_cb_t {
  M4 view;
  M4 proj;
  M4 light_view;
  M4 light_proj;
  V4 eye_pos;
  V4 obj_color;
  V4 light_pos;
  V4 light_color;
};

struct ui_cb_t {
  F32 width;
  F32 height;
};

struct dx12_descriptor_heap {
  ID3D12DescriptorHeap* heap = NULL;
  U32 increment_size = 0;
  U32 curr_index = 0;
};

struct dx12_descriptor {
  dx12_descriptor_heap* descriptor_heap = NULL;
  U32 index = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
};

struct dx12_buffer {
  ID3D12Resource* buffer = NULL;
  void* cpu_p = NULL;
  SIP offset = 0;
};

struct dx12_subbuffer {
  dx12_buffer* buffer = NULL;
  U8* cpu_p = NULL;
  D3D12_GPU_VIRTUAL_ADDRESS gpu_p = 0;
  SIP offset = 0;
  SIP size = 0;
};

struct DX12Window : public ngWindow {
  DX12Window(const OSChar* title, int w, int h) : ngWindow(title, w, h) {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(EMouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;
  void on_key_event(EKey key, bool is_down) override;
  void on_char_event(wchar_t c) override;

  void wait_for_gpu();

  ngCam m_cam;

  static const int sc_frame_count = 2;
  int m_frame_no;
  ID3D12Device* m_device = NULL;
  ID3D12CommandQueue* m_cmd_queue = NULL;
  IDXGISwapChain3* m_swap_chain = NULL;

  dx12_descriptor_heap m_rtv_heap;
  ID3D12Resource* m_render_targets[sc_frame_count];
  dx12_descriptor m_rtv_descriptors[sc_frame_count];

  ID3D12CommandAllocator* m_cmd_allocators[sc_frame_count];
  ID3D12GraphicsCommandList* m_cmd_list;

  dx12_descriptor_heap m_cbv_srv_heap;
  dx12_buffer m_upload_buffer;
  dx12_descriptor m_shadow_srv_descriptor;

  dx12_descriptor m_per_obj_cbv_descriptors[10];
  dx12_subbuffer m_per_obj_cb_subbuffers[10];
  per_obj_cb_t m_per_obj_cbs[10];

  dx12_descriptor m_shadow_shared_cbv_descriptor;
  dx12_subbuffer m_shadow_shared_cb_subbuffer;
  shadow_shared_cb_t m_shadow_shared_cb;

  dx12_descriptor m_final_shared_cbv_descriptor;
  dx12_subbuffer m_final_shared_cb_subbuffer;
  final_shared_cb_t m_final_shared_cb;

  dx12_descriptor m_ui_cbv_descriptor;
  dx12_subbuffer m_ui_cb_subbuffer;
  ui_cb_t m_ui_cb;

  dx12_descriptor_heap m_dsv_heap;
  ID3D12Resource* m_depth_stencil;
  ID3D12Resource* m_shadow_depth_stencil;
  dx12_descriptor m_depth_rt_descriptor;
  dx12_descriptor m_shadow_depth_rt_descriptor;

  ID3D12RootSignature* m_shadow_root_sig;
  ID3D12RootSignature* m_final_root_sig;
  ID3D12RootSignature* m_ui_root_sig;
  ID3DBlob* m_final_vs;
  ID3DBlob* m_final_ps;
  ID3DBlob* m_shadow_vs;
  ID3DBlob* m_ui_vs;
  ID3DBlob* m_ui_ps;

  ID3D12PipelineState* m_shadow_pso;
  ID3D12PipelineState* m_final_pso;
  ID3D12PipelineState* m_ui_pso;

  dx12_buffer m_vertex_buffer;
  dx12_subbuffer m_vertices_subbuffer;
  dx12_subbuffer m_normals_subbuffer;
  dx12_subbuffer m_text_subbuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertices_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_normals_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_ui_vb_view;
  U32 m_objs_count = 0;
  U32 m_obj_vertices_nums[10] = {};
  dx12_subbuffer m_texture_subbuffer;
  ID3D12Resource* m_texture;
  dx12_descriptor m_texture_srv_descriptor;
  U32 m_visible_text_len = 0;

  const U32 m_normals_stride = 64 * 1024 * 1024;

  ID3D12Fence* m_fence;
  HANDLE m_fence_event;
  U64 m_fence_vals[sc_frame_count] = {};

  S64 m_start_time;
  bool m_is_console_active = false;
};

static bool create_descriptor_heap(dx12_descriptor_heap* descriptor_heap, ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, U32 max_descriptors_num) {
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = type;
  desc.NumDescriptors = max_descriptors_num;
  desc.Flags = flags;
  desc.NodeMask = 0;
  DX_CHECK_RETURN_FALSE(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap->heap)));
  descriptor_heap->increment_size = device->GetDescriptorHandleIncrementSize(type);
  return true;
}

static dx12_descriptor allocate_descriptor(dx12_descriptor_heap* descriptor_heap) {
  dx12_descriptor descriptor;
  CHECK_LOG_RETURN_VAL(descriptor_heap->curr_index < descriptor_heap->heap->GetDesc().NumDescriptors, descriptor, "Out of descriptors");
  descriptor.descriptor_heap = descriptor_heap;
  descriptor.index = descriptor_heap->curr_index;
  descriptor.cpu_handle.ptr = descriptor_heap->heap->GetCPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  descriptor.gpu_handle.ptr = descriptor_heap->heap->GetGPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  ++descriptor_heap->curr_index;
  return descriptor;
}

static D3D12_RESOURCE_DESC create_resource_desc(size_t size) {
  D3D12_RESOURCE_DESC desc = {};
  desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  desc.Alignment = 0;
  desc.Width = size;
  desc.Height = 1;
  desc.DepthOrArraySize = 1;
  desc.MipLevels = 1;
  desc.Format = DXGI_FORMAT_UNKNOWN;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  return desc;
}

static D3D12_HEAP_PROPERTIES create_heap_props(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES props = {};
  props.Type = type;
  props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  props.CreationNodeMask = 1;
  props.VisibleNodeMask = 1;
  return props;
}

static D3D12_DESCRIPTOR_RANGE1 create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE range_type,
                                                           UINT num_descriptors,
                                                           UINT base_shader_reg,
                                                           UINT reg_space,
                                                           D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                                           UINT offset_in_descriptors_from_table_start = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
  D3D12_DESCRIPTOR_RANGE1 range = {};
  range.RangeType = range_type;
  range.NumDescriptors = num_descriptors;
  range.BaseShaderRegister = base_shader_reg;
  range.RegisterSpace = reg_space;
  range.Flags = flags;
  range.OffsetInDescriptorsFromTableStart = offset_in_descriptors_from_table_start;
  return range;
}

static D3D12_ROOT_PARAMETER1
create_root_param_1_1_descriptor_table(UINT num_ranges, const D3D12_DESCRIPTOR_RANGE1* ranges, D3D12_SHADER_VISIBILITY shader_visibility) {
  D3D12_ROOT_PARAMETER1 param = {};
  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  param.DescriptorTable = {};
  param.DescriptorTable.NumDescriptorRanges = num_ranges;
  param.DescriptorTable.pDescriptorRanges = ranges;
  param.ShaderVisibility = shader_visibility;
  return param;
}

static D3D12_VERSIONED_ROOT_SIGNATURE_DESC create_root_sig_desc_1_1(UINT num_root_params,
                                                                    const D3D12_ROOT_PARAMETER1* root_params,
                                                                    UINT num_static_samplers,
                                                                    const D3D12_STATIC_SAMPLER_DESC* static_samplers,
                                                                    D3D12_ROOT_SIGNATURE_FLAGS flags) {
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
  desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  desc.Desc_1_1.NumParameters = num_root_params;
  desc.Desc_1_1.pParameters = root_params;
  desc.Desc_1_1.NumStaticSamplers = num_static_samplers;
  desc.Desc_1_1.pStaticSamplers = static_samplers;
  desc.Desc_1_1.Flags = flags;
  return desc;
}

static D3D12_RESOURCE_BARRIER create_transition_barrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  return barrier;
}

static D3D12_CONSTANT_BUFFER_VIEW_DESC create_const_buf_view_desc(D3D12_GPU_VIRTUAL_ADDRESS location, UINT size) {
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  desc.BufferLocation = location;
  desc.SizeInBytes = size;
  return desc;
}

static bool create_buffer(dx12_buffer* buffer, ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap_props, const D3D12_RESOURCE_DESC* desc) {
  *buffer = {};
  DX_CHECK_RETURN_FALSE(device->CreateCommittedResource(heap_props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buffer->buffer)));
  buffer->buffer->Map(0, NULL, &buffer->cpu_p);
  return true;
}

static dx12_subbuffer allocate_subbuffer(dx12_buffer* buffer, SIP size, SIP alignment) {
  dx12_subbuffer subbuffer = {};
  SIP aligned_offset = (buffer->offset + alignment - 1) & ~(alignment - 1);
  CHECK_LOG_RETURN_VAL(aligned_offset + size <= buffer->buffer->GetDesc().Width, subbuffer, "Out of memory");
  subbuffer.buffer = buffer;
  subbuffer.cpu_p = (U8*)buffer->cpu_p + aligned_offset;
  subbuffer.gpu_p = buffer->buffer->GetGPUVirtualAddress() + aligned_offset;
  subbuffer.offset = aligned_offset;
  subbuffer.size = size;
  buffer->offset = aligned_offset + size;
  return subbuffer;
}

bool compile_shader(const OSChar* path, const char* entry, const char* target, ID3DBlob** shader) {
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  ID3DBlob* error;
  if (D3DCompileFromFile(path, NULL, NULL, entry, target, compile_flags, 0, shader, &error) != S_OK) {
    LOGF("%s", (const char*)error->GetBufferPointer());
    return false;
  }
  return true;
}

bool DX12Window::init() {
  ngWindow::init();

  m_start_time = mono_time_now();
  m_cam.cam_init({5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
  m_final_shared_cb.eye_pos = V3o_v4(m_cam.m_eye, 1.0f);
  m_final_shared_cb.obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
  m_final_shared_cb.light_pos = {10.0f, 10.0f, 10.0f, 1.0f};
  m_final_shared_cb.light_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // The light is static for now.
  ngCam light_cam;
  light_cam.cam_init({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, this);
  // TODO: ortho?
  M4 perspective_m4 = perspective(degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);

  m_shadow_shared_cb.light_view = light_cam.m_view_mat;
  m_shadow_shared_cb.light_proj = perspective_m4;

  m_final_shared_cb.proj = perspective_m4;
  m_final_shared_cb.light_view = light_cam.m_view_mat;
  m_final_shared_cb.light_proj = perspective_m4;

  m_ui_cb.width = (F32)m_width;
  m_ui_cb.height = (F32)m_height;

  UINT dxgi_factory_flags = 0;
  {
    // Enable debug layer.
    ID3D12Debug* debug_controller;
    DX_CHECK_RETURN_FALSE(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer();
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    debug_controller->Release();
  }

  IDXGIFactory4* dxgi_factory;
  DX_CHECK_RETURN_FALSE(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

  {
    // Choose adapter (graphics card).
    IDXGIAdapter1* adapter;
    int backup_adapter_i = -1;
    int adapter_i = -1;
    for (int i = 0; dxgi_factory->EnumAdapters1(i, &adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);
      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        continue;

      if (wcsstr(desc.Description, L"Intel")) {
        backup_adapter_i = i;
        continue;
      }

      adapter_i = i;
      break;
    }

    if (adapter_i == -1) {
      CHECK_LOG_RETURN_VAL(backup_adapter_i != -1, false, "Can't find a dx12 adapter");
      adapter_i = backup_adapter_i;
    }

    dxgi_factory->EnumAdapters1(adapter_i, &adapter);
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    LOGI("%ls", desc.Description);
    DX_CHECK_RETURN_FALSE(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
  }

  {
    D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
    cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    DX_CHECK_RETURN_FALSE(m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue)));
  }

  DXGI_SWAP_CHAIN_DESC1 sc_desc = {};
  sc_desc.BufferCount = sc_frame_count;
  sc_desc.Width = m_width;
  sc_desc.Height = m_height;
  sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sc_desc.SampleDesc.Count = 1;

  IDXGISwapChain1* swap_chain;
  DX_CHECK_RETURN_FALSE(dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue, m_platform_data.hwnd, &sc_desc, NULL, NULL, &swap_chain));
  DX_CHECK_RETURN_FALSE(dxgi_factory->MakeWindowAssociation(m_platform_data.hwnd, DXGI_MWA_NO_ALT_ENTER));
  DX_CHECK_RETURN_FALSE(swap_chain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)));
  dxgi_factory->Release();
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();

  {
    if (!create_descriptor_heap(&m_rtv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, sc_frame_count))
      return false;

    for (int i = 0; i < sc_frame_count; ++i) {
      DX_CHECK_RETURN_FALSE(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
      m_rtv_descriptors[i] = allocate_descriptor(&m_rtv_heap);
      m_device->CreateRenderTargetView(m_render_targets[i], NULL, m_rtv_descriptors[i].cpu_handle);
    }
  }

  {
    if (!create_descriptor_heap(&m_dsv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2))
      return false;

    D3D12_RESOURCE_DESC depth_tex_desc = {};
    depth_tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depth_tex_desc.Alignment = 0;
    depth_tex_desc.Width = m_width;
    depth_tex_desc.Height = m_height;
    depth_tex_desc.DepthOrArraySize = 1;
    depth_tex_desc.MipLevels = 1;
    depth_tex_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
    depth_tex_desc.SampleDesc.Count = 1;
    depth_tex_desc.SampleDesc.Quality = 0;
    depth_tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depth_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE clear_value;
    clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clear_value.DepthStencil.Depth = 1.0f;
    clear_value.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES heap_props = create_heap_props(D3D12_HEAP_TYPE_DEFAULT);

    DX_CHECK_RETURN_FALSE(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&m_depth_stencil)));
    m_depth_rt_descriptor = allocate_descriptor(&m_dsv_heap);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_view_desc = {};
    dsv_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_view_desc.Texture2D.MipSlice = 0;
    m_device->CreateDepthStencilView(m_depth_stencil, &dsv_view_desc, m_depth_rt_descriptor.cpu_handle);

    DX_CHECK_RETURN_FALSE(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_GENERIC_READ, &clear_value, IID_PPV_ARGS(&m_shadow_depth_stencil)));
    m_shadow_depth_rt_descriptor = allocate_descriptor(&m_dsv_heap);
    m_device->CreateDepthStencilView(m_shadow_depth_stencil, &dsv_view_desc, m_shadow_depth_rt_descriptor.cpu_handle);
  }

  {
    if(!create_descriptor_heap(&m_cbv_srv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10))
      return false;

    {
      m_shadow_srv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
      // Allocate here and use later so they will be next to each other.
      m_texture_srv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
      D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
      srv_desc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
      srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srv_desc.Texture2D.MostDetailedMip = 0;
      srv_desc.Texture2D.MipLevels = 1;
      srv_desc.Texture2D.PlaneSlice = 0;
      srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
      m_device->CreateShaderResourceView(m_shadow_depth_stencil, &srv_desc, m_shadow_srv_descriptor.cpu_handle);
    }

    // Constant buffer
    D3D12_RESOURCE_DESC upload_desc = create_resource_desc(64 * 1024 * 1024);
    D3D12_HEAP_PROPERTIES heap_props = create_heap_props(D3D12_HEAP_TYPE_UPLOAD);
    if (!create_buffer(&m_upload_buffer, m_device, &heap_props, &upload_desc))
      return false;
    // From D3D12HelloConstantBuffers sample
    // CB size is required to be 256-byte aligned
    {
      m_shadow_shared_cbv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
      SIP cb_size = (sizeof(shadow_shared_cb_t) + 255) & ~255;
      m_shadow_shared_cb_subbuffer = allocate_subbuffer(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_shadow_shared_cb_subbuffer.gpu_p, m_shadow_shared_cb_subbuffer.size), m_shadow_shared_cbv_descriptor.cpu_handle);
      memcpy(m_shadow_shared_cb_subbuffer.cpu_p, &m_shadow_shared_cb, sizeof(m_shadow_shared_cb));
    }

    {
      m_final_shared_cbv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
      SIP cb_size = (sizeof(final_shared_cb_t) + 255) & ~255;
      m_final_shared_cb_subbuffer = allocate_subbuffer(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_final_shared_cb_subbuffer.gpu_p, m_final_shared_cb_subbuffer.size), m_final_shared_cbv_descriptor.cpu_handle);
    }

    {
      m_ui_cbv_descriptor = allocate_descriptor(&m_cbv_srv_heap);
      SIP cb_size = (sizeof(ui_cb_t) + 255) & ~255;
      m_ui_cb_subbuffer = allocate_subbuffer(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_ui_cb_subbuffer.gpu_p, m_ui_cb_subbuffer.size), m_ui_cbv_descriptor.cpu_handle);
      memcpy(m_ui_cb_subbuffer.cpu_p, &m_ui_cb, sizeof(m_ui_cb));
    }
  }

  {
    // Create a root signature consisting of a descriptor table with a single CBV
    D3D12_DESCRIPTOR_RANGE1 ranges[] = {
      create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
      create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0),
    };
    D3D12_ROOT_PARAMETER1 root_params[] = {
      create_root_param_1_1_descriptor_table(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX),
      create_root_param_1_1_descriptor_table(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX),
    };
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc = create_root_sig_desc_1_1(
        static_array_size(root_params),
        root_params,
        0,
        NULL,
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
            D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
    ID3DBlob* signature;
    ID3DBlob* error;
    DX_CHECK_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
    DX_CHECK_RETURN_FALSE(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_shadow_root_sig)));
  }

  {
    OSChar shader_path[MAX_PATH_LEN];
    path_from_exe_dir(shader_path, OS_TXT("assets/shadow.hlsl"), MAX_PATH_LEN);
    compile_shader(shader_path, "VSMain", "vs_5_0", &m_shadow_vs);

    path_from_exe_dir(shader_path, OS_TXT("assets/shader.hlsl"), MAX_PATH_LEN);
    compile_shader(shader_path, "VSMain", "vs_5_0", &m_final_vs);
    compile_shader(shader_path, "PSMain", "ps_5_0", &m_final_ps);

    path_from_exe_dir(shader_path, OS_TXT("assets/ui.hlsl"), MAX_PATH_LEN);
    compile_shader(shader_path, "VSMain", "vs_5_0", &m_ui_vs);
    compile_shader(shader_path, "PSMain", "ps_5_0", &m_ui_ps);
  }

  {
    D3D12_RASTERIZER_DESC rasterizer_desc = {};
    rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
    rasterizer_desc.FrontCounterClockwise = TRUE;
    rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable = TRUE;
    rasterizer_desc.MultisampleEnable = FALSE;
    rasterizer_desc.AntialiasedLineEnable = FALSE;
    rasterizer_desc.ForcedSampleCount = 0;
    rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    D3D12_BLEND_DESC blend_desc = {};
    blend_desc.AlphaToCoverageEnable = FALSE;
    blend_desc.IndependentBlendEnable = FALSE;
    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
      D3D12_RENDER_TARGET_BLEND_DESC* rt_desc = &blend_desc.RenderTarget[i];
      *rt_desc = {};
      rt_desc->BlendEnable = TRUE;
      rt_desc->LogicOpEnable = FALSE;
      rt_desc->SrcBlend = D3D12_BLEND_SRC_ALPHA;
      rt_desc->DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
      rt_desc->BlendOp = D3D12_BLEND_OP_ADD;
      rt_desc->SrcBlendAlpha = D3D12_BLEND_ONE;
      rt_desc->DestBlendAlpha =D3D12_BLEND_INV_SRC_ALPHA;
      rt_desc->BlendOpAlpha = D3D12_BLEND_OP_ADD;
      rt_desc->LogicOp = D3D12_LOGIC_OP_NOOP;
      rt_desc->RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    D3D12_INPUT_ELEMENT_DESC input_elem_descs[] = {
      { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC shadow_pso_desc = {};
      shadow_pso_desc.pRootSignature = m_shadow_root_sig;
      shadow_pso_desc.VS.pShaderBytecode = m_shadow_vs->GetBufferPointer();
      shadow_pso_desc.VS.BytecodeLength = m_shadow_vs->GetBufferSize();
      shadow_pso_desc.BlendState = blend_desc;
      shadow_pso_desc.SampleMask = UINT_MAX;
      shadow_pso_desc.RasterizerState = rasterizer_desc;
      shadow_pso_desc.DepthStencilState.DepthEnable = TRUE;
      shadow_pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      shadow_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      shadow_pso_desc.DepthStencilState.StencilEnable = FALSE;
      shadow_pso_desc.InputLayout.pInputElementDescs = input_elem_descs;
      shadow_pso_desc.InputLayout.NumElements = (UINT)static_array_size(input_elem_descs);
      shadow_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      shadow_pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      shadow_pso_desc.NumRenderTargets = 0;
      shadow_pso_desc.SampleDesc.Count = 1;
      DX_CHECK_RETURN_FALSE(m_device->CreateGraphicsPipelineState(&shadow_pso_desc, IID_PPV_ARGS(&m_shadow_pso)));
    }

    for (int i = 0; i < sc_frame_count; ++i)
      DX_CHECK_RETURN_FALSE(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocators[i])));
    DX_CHECK_RETURN_FALSE(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocators[m_frame_no], m_shadow_pso, IID_PPV_ARGS(&m_cmd_list)));

    D3D12_INPUT_ELEMENT_DESC final_elem_descs[] = {
      { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      { "N", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL),
          create_root_param_1_1_descriptor_table(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL),
          create_root_param_1_1_descriptor_table(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL),
      };
      D3D12_STATIC_SAMPLER_DESC sampler = {};
      sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.MipLODBias = 0;
      sampler.MaxAnisotropy = 0;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = 0;
      sampler.RegisterSpace = 0;
      sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc =
          create_root_sig_desc_1_1(static_array_size(root_params), root_params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
      ID3DBlob* signature;
      ID3DBlob* error;
      DX_CHECK_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
      DX_CHECK_RETURN_FALSE(
          m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_final_root_sig)));
    }

    {
      D3D12_GRAPHICS_PIPELINE_STATE_DESC final_pso_desc = {};
      final_pso_desc.pRootSignature = m_final_root_sig;
      final_pso_desc.VS.pShaderBytecode = m_final_vs->GetBufferPointer();
      final_pso_desc.VS.BytecodeLength = m_final_vs->GetBufferSize();
      final_pso_desc.PS.pShaderBytecode = m_final_ps->GetBufferPointer();
      final_pso_desc.PS.BytecodeLength = m_final_ps->GetBufferSize();
      final_pso_desc.BlendState = blend_desc;
      final_pso_desc.SampleMask = UINT_MAX;
      final_pso_desc.RasterizerState = rasterizer_desc;
      final_pso_desc.DepthStencilState.DepthEnable = TRUE;
      final_pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      final_pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      final_pso_desc.DepthStencilState.StencilEnable = FALSE;
      final_pso_desc.InputLayout.pInputElementDescs = final_elem_descs;
      final_pso_desc.InputLayout.NumElements = (UINT)static_array_size(final_elem_descs);
      final_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      final_pso_desc.NumRenderTargets = 1;
      final_pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
      final_pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
      final_pso_desc.SampleDesc.Count = 1;
      DX_CHECK_RETURN_FALSE(m_device->CreateGraphicsPipelineState(&final_pso_desc, IID_PPV_ARGS(&m_final_pso)));
    }

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
        create_descriptor_range_1_1(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL),
          create_root_param_1_1_descriptor_table(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX),
      };
      D3D12_STATIC_SAMPLER_DESC sampler = {};
      sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
      sampler.MipLODBias = 0;
      sampler.MaxAnisotropy = 0;
      sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
      sampler.MinLOD = 0.0f;
      sampler.MaxLOD = D3D12_FLOAT32_MAX;
      sampler.ShaderRegister = 0;
      sampler.RegisterSpace = 0;
      sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_sig_desc =
          create_root_sig_desc_1_1(static_array_size(root_params), root_params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
      ID3DBlob* signature;
      ID3DBlob* error;
      DX_CHECK_RETURN_FALSE(D3D12SerializeVersionedRootSignature(&root_sig_desc, &signature, &error));
      DX_CHECK_RETURN_FALSE(
          m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_ui_root_sig)));
    }

    {
      D3D12_INPUT_ELEMENT_DESC ui_elem_descs[] = {
        { "V", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(F32), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      };

      D3D12_GRAPHICS_PIPELINE_STATE_DESC ui_pso_desc = {};
      ui_pso_desc.pRootSignature = m_ui_root_sig;
      ui_pso_desc.VS.pShaderBytecode = m_ui_vs->GetBufferPointer();
      ui_pso_desc.VS.BytecodeLength = m_ui_vs->GetBufferSize();
      ui_pso_desc.PS.pShaderBytecode = m_ui_ps->GetBufferPointer();
      ui_pso_desc.PS.BytecodeLength = m_ui_ps->GetBufferSize();
      ui_pso_desc.BlendState = blend_desc;
      ui_pso_desc.SampleMask = UINT_MAX;
      ui_pso_desc.RasterizerState = rasterizer_desc;
      ui_pso_desc.DepthStencilState = {};
      ui_pso_desc.InputLayout.pInputElementDescs = ui_elem_descs;
      ui_pso_desc.InputLayout.NumElements = (UINT)static_array_size(ui_elem_descs);
      ui_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
      ui_pso_desc.NumRenderTargets = 1;
      ui_pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
      ui_pso_desc.SampleDesc.Count = 1;
      DX_CHECK_RETURN_FALSE(m_device->CreateGraphicsPipelineState(&ui_pso_desc, IID_PPV_ARGS(&m_ui_pso)));
    }
  }

  {
    stbtt_fontinfo font;
    OSChar font_path[MAX_PATH_LEN];
    path_from_exe_dir(font_path, OS_TXT("assets/UbuntuMono-Regular.ttf"), MAX_PATH_LEN);
    DynamicArray<U8> font_buf = ngFile::f_read_whole_file_as_text(g_persistent_allocator, font_path);
    CHECK_RETURN_VAL(stbtt_InitFont(&font, &font_buf[0], stbtt_GetFontOffsetForIndex(&font_buf[0], 0)) != 0, false);
    g_font.fontinfo = font;
    const int c_tex_w = 2048;
    const int c_tex_h = 2048;
    LinearAllocator<> temp_allocator("font_allocator");
    temp_allocator.la_init();
    SCOPE_EXIT(temp_allocator.al_destroy());
    U8* font_tex = (U8*)temp_allocator.al_alloc_zero(c_tex_w * c_tex_h);
    int font_h = 16;
    float scale = stbtt_ScaleForPixelHeight(&font, font_h);
    g_font.scale = scale;
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &line_gap);
    g_font.baseline = ascent * scale;
    g_font.line_space = (ascent - descent + line_gap) * scale;
    const float c_x_left = 2.0f; // leave a little padding in case the character extends left
    float x = c_x_left;
    int y = 0;
    for (int c = 0; c <= 16*1024; ++c) {
      // if (!isprint(c))
      //   continue;
      float x_shift = x - (float) floor(x);
      int x0, y0, x1, y1;
      stbtt_GetCodepointBitmapBoxSubpixel(&font, c, scale, scale, x_shift, 0.0f, &x0, &y0, &x1, &y1);
      int w = x1 - x0;
      int h = y1 - y0;
      CHECK_LOG_RETURN_VAL(y + h < c_tex_h, false, "Bigger than texture height");
      if (x + w + 1 >= c_tex_w) {
        y += font_h;
        x = c_x_left;
      }
      // Is +1 necessary for subpixel rendering?
      CHECK_LOG_RETURN_VAL(x + w + 1 < c_tex_h, false, "Bigger than texture width");
      stbtt_MakeCodepointBitmapSubpixel(&font, &font_tex[(int)(y) * c_tex_w + (int)x], w, h, c_tex_w, scale, scale, x_shift, 0.0f, c);
      int advance, lsb;
      stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
      g_font.codepoints[c].uv_top_left = V2{x + 0.5f, y + 0.5f} / V2{c_tex_w, c_tex_h};
      g_font.codepoints[c].uv_bottom_right = V2{x + w - 0.5f, y + h - 0.5f} / V2{c_tex_w, c_tex_h};
      g_font.codepoints[c].advance = advance;
      g_font.codepoints[c].lsb = lsb;
      g_font.codepoints[c].x0 = x0;
      g_font.codepoints[c].x1 = x1;
      g_font.codepoints[c].y0 = y0;
      g_font.codepoints[c].y1 = y1;
      x += w;
    }

    D3D12_SUBRESOURCE_FOOTPRINT pitched_desc = {};
    pitched_desc.Format = DXGI_FORMAT_R8_UNORM;
    pitched_desc.Width = c_tex_w;
    pitched_desc.Height = c_tex_h;
    pitched_desc.Depth = 1;
    pitched_desc.RowPitch = (c_tex_w + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    m_texture_subbuffer = allocate_subbuffer(&m_upload_buffer, pitched_desc.Height * pitched_desc.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_texture = {};
    placed_texture.Offset = (SIP)m_texture_subbuffer.cpu_p - (SIP)m_texture_subbuffer.buffer->cpu_p;
    placed_texture.Footprint = pitched_desc;
    for (int i = 0; i < pitched_desc.Height; ++i) {
      memcpy(m_texture_subbuffer.cpu_p + i * pitched_desc.RowPitch, &font_tex[i * c_tex_w], pitched_desc.Width);
    }
    D3D12_RESOURCE_DESC tex_desc = {};
    tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    tex_desc.Alignment = 0;
    tex_desc.Width = c_tex_w;
    tex_desc.Height = c_tex_h;
    tex_desc.DepthOrArraySize = 1;
    tex_desc.MipLevels = 1;
    tex_desc.Format = DXGI_FORMAT_R8_UNORM;
    tex_desc.SampleDesc.Count = 1;
    tex_desc.SampleDesc.Quality = 0;
    tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    tex_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    DX_CHECK_RETURN_FALSE(m_device->CreateCommittedResource(&create_heap_props(D3D12_HEAP_TYPE_DEFAULT),
                                                            D3D12_HEAP_FLAG_NONE,
                                                            &tex_desc,
                                                            D3D12_RESOURCE_STATE_COPY_DEST,
                                                            NULL,
                                                            IID_PPV_ARGS(&m_texture)));
    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource = m_texture_subbuffer.buffer->buffer;
    src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = placed_texture;
    D3D12_TEXTURE_COPY_LOCATION dest = {};
    dest.pResource = m_texture;
    dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dest.SubresourceIndex = 0;
    m_cmd_list->CopyTextureRegion(&dest, 0, 0, 0, &src, NULL);
    m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
    m_cmd_list->Close();
    m_cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_cmd_list);

    {
      D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
      srv_desc.Format = DXGI_FORMAT_R8_UNORM;
      srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
      srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      srv_desc.Texture2D.MostDetailedMip = 0;
      srv_desc.Texture2D.MipLevels = 1;
      srv_desc.Texture2D.PlaneSlice = 0;
      srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
      m_device->CreateShaderResourceView(m_texture, &srv_desc, m_texture_srv_descriptor.cpu_handle);
    }
  }

  {
    LinearAllocator<> temp_allocator("obj_allocator");
    temp_allocator.la_init();
    SCOPE_EXIT(temp_allocator.al_destroy());
    const OSChar* obj_paths[] = {
        OS_TXT("assets/plane.obj"),
        OS_TXT("assets/wolf.obj"),
    };
    SIP vertices_offset = 0;
    SIP normals_offset = 0;
    m_objs_count = static_array_size(obj_paths);

    {
      SIP cb_size = (sizeof(per_obj_cb_t) + 255) & ~255;
      for (int i = 0; i < m_objs_count; ++i) {
        m_per_obj_cb_subbuffers[i] = allocate_subbuffer(&m_upload_buffer, cb_size, 256);
        m_per_obj_cbv_descriptors[i] = allocate_descriptor(&m_cbv_srv_heap);
        m_device->CreateConstantBufferView(&create_const_buf_view_desc(m_per_obj_cb_subbuffers[i].gpu_p, cb_size), m_per_obj_cbv_descriptors[i].cpu_handle);
        m_per_obj_cbs[i].world = m4_identity();
        memcpy(m_per_obj_cb_subbuffers[i].cpu_p, &m_per_obj_cbs[i], sizeof(per_obj_cb_t));
      }
    }

    if (!create_buffer(&m_vertex_buffer, m_device, &create_heap_props(D3D12_HEAP_TYPE_UPLOAD), &create_resource_desc(128 * 1024 * 1024)))
      return false;

    m_vertices_subbuffer = allocate_subbuffer(&m_vertex_buffer, 16 * 1024 * 1024, 16);
    m_normals_subbuffer = allocate_subbuffer(&m_vertex_buffer, 16 * 1024 * 1024, 16);
    m_text_subbuffer = allocate_subbuffer(&m_vertex_buffer, 1024 * 1024, 16);
    for (int i = 0; i < m_objs_count; ++i) {
      OBJLoader obj;
      OSChar full_obj_path[MAX_PATH_LEN];
      obj.obj_init(&temp_allocator, path_from_exe_dir(full_obj_path, obj_paths[i], MAX_PATH_LEN));
      m_obj_vertices_nums[i] = obj.m_vertices.da_len();
      int vertices_size = m_obj_vertices_nums[i] * sizeof(obj.m_vertices[0]);
      int normals_size = m_obj_vertices_nums[i] * sizeof(obj.m_normals[0]);
      CHECK_RETURN_VAL(vertices_offset + vertices_size <= m_vertices_subbuffer.size, false);
      CHECK_RETURN_VAL(normals_offset + normals_size <= m_normals_subbuffer.size, false);
      memcpy(m_vertices_subbuffer.cpu_p + vertices_offset, &obj.m_vertices[0], vertices_size);
      memcpy(m_normals_subbuffer.cpu_p + normals_offset, &obj.m_normals[0], normals_size);
      vertices_offset += vertices_size;
      normals_offset += normals_size;
    }
    m_vertices_vb_view.BufferLocation = m_vertices_subbuffer.gpu_p;
    m_vertices_vb_view.SizeInBytes = vertices_offset;
    m_vertices_vb_view.StrideInBytes = sizeof(((OBJLoader*)0)->m_vertices[0]);

    m_normals_vb_view.BufferLocation = m_normals_subbuffer.gpu_p;
    m_normals_vb_view.SizeInBytes = normals_offset;
    m_normals_vb_view.StrideInBytes = sizeof(((OBJLoader*)0)->m_normals[0]);

    m_vertex_buffer.buffer->Unmap(0, NULL);
  }

  {
    DX_CHECK_RETURN_FALSE(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_vals[m_frame_no];
    m_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    CHECK_RETURN_VAL(m_fence_event, false);
    wait_for_gpu();
  }

  return true;
}

void DX12Window::destroy() {
  if (m_device)
    m_device->Release();
  if (m_cmd_queue)
    m_cmd_queue->Release();
}

void DX12Window::loop() {
  {
    LinearAllocator<> temp_allocator("text_allocator");
    temp_allocator.la_init();
    SCOPE_EXIT(temp_allocator.al_destroy());
    m_vertex_buffer.buffer->Map(0, NULL, &m_vertex_buffer.cpu_p);
    // text data
    static S64 last_frametime = mono_time_now();
    S64 now = mono_time_now();
    char text[256];
    F64 frametime = mono_time_to_ms(now - last_frametime);
    float fps = (int)(1000.0 / frametime);
    int i_fps;
    if (fps - (int)fps > 0.5)
      i_fps = (int)fps + 1;
    else
      i_fps = (int)fps;
    snprintf(text, 256, "Frametime: %.2fms\nFPS: %d", frametime, i_fps);
    last_frametime = now;
    // Console background
    int c_console_height = 600;
    int max_num_lines = c_console_height / g_font.line_space + 1;

    DynamicArray<float> ui_data;
    ui_data.da_init(&temp_allocator);
    const float c_x_left = 10.0f;
    const float c_max_w = 600.0f;
    const float c_first_line = 400.0f;
    float x = c_x_left;
    float y = c_first_line;
    int text_len = strlen(text);
    m_visible_text_len = 0;
    float scale = g_font.scale;
    for (int i = 0; i < text_len; ++i) {
      const char* c = &text[i];
      if (*c == '\n') {
        y += g_font.line_space;
        x= c_x_left;
        continue;
      }
      ++m_visible_text_len;
      const char* nc = &text[i + 1];
      codepoint_t* cp = &g_font.codepoints[*c];
      float left = x + cp->x0;
      float right = x + cp->x1;
      if (right > c_max_w) {
        y += g_font.line_space;
        x = c_x_left;
        left = x + cp->x0;
        right = x + cp->x1;
      }
      float top = y - g_font.baseline + cp->y0;
      float bottom = y - g_font.baseline + cp->y1;
      ui_data.da_append(left);
      ui_data.da_append(top);
      ui_data.da_append(cp->uv_top_left.x);
      ui_data.da_append(cp->uv_top_left.y);

      ui_data.da_append(left);
      ui_data.da_append(bottom);
      ui_data.da_append(cp->uv_top_left.x);
      ui_data.da_append(cp->uv_bottom_right.y);

      ui_data.da_append(right);
      ui_data.da_append(top);
      ui_data.da_append(cp->uv_bottom_right.x);
      ui_data.da_append(cp->uv_top_left.y);

      ui_data.da_append(right);
      ui_data.da_append(top);
      ui_data.da_append(cp->uv_bottom_right.x);
      ui_data.da_append(cp->uv_top_left.y);

      ui_data.da_append(left);
      ui_data.da_append(bottom);
      ui_data.da_append(cp->uv_top_left.x);
      ui_data.da_append(cp->uv_bottom_right.y);

      ui_data.da_append(right);
      ui_data.da_append(bottom);
      ui_data.da_append(cp->uv_bottom_right.x);
      ui_data.da_append(cp->uv_bottom_right.y);

      x += scale * cp->advance;
      if (nc) {
         x += scale * stbtt_GetCodepointKernAdvance(&g_font.fontinfo, *c, *nc);
      }
      if (isspace(*c) && !isspace(*nc)) {
        float word_length = 0.0f;
        while (*nc && !isspace(*nc)) {
          word_length += scale * stbtt_GetCodepointKernAdvance(&g_font.fontinfo, *(nc - 1), *nc);
          word_length += scale * g_font.codepoints[*nc].advance;
          ++nc;
        }
        if (x + word_length > c_max_w) {
          y += g_font.line_space;
          x = c_x_left;
        }
      }
    }

    memcpy((U8*)m_vertex_buffer.cpu_p + m_text_subbuffer.offset, &ui_data[0], ui_data.da_len() * sizeof(float));
    m_ui_vb_view.BufferLocation = m_vertex_buffer.buffer->GetGPUVirtualAddress() + m_text_subbuffer.offset;
    m_ui_vb_view.SizeInBytes = ui_data.da_len() * sizeof(float);
    m_ui_vb_view.StrideInBytes = 4 * sizeof(float);
    m_vertex_buffer.buffer->Unmap(0, NULL);
  }

  m_cam.cam_update();
  m_final_shared_cb.view = m_cam.m_view_mat;
  memcpy(m_final_shared_cb_subbuffer.cpu_p, &m_final_shared_cb, sizeof(m_final_shared_cb));

  F64 delta_s = mono_time_to_s(mono_time_now() - m_start_time);
  F64 loop_s = 4.0;
  F64 mod = fmod(delta_s, loop_s);
  F64 norm_mod = mod / loop_s;
  if (norm_mod > 0.5)
    norm_mod = 1.0 - norm_mod;
  F32 from_y = -0.5f;
  F32 to_y = 0.5f;
  F32 y = (to_y - from_y) * norm_mod * 2 + from_y;
  m_per_obj_cbs[1].world = translate({0.0f, y, 0.0f});
  memcpy(m_per_obj_cb_subbuffers[1].cpu_p, &m_per_obj_cbs[1], sizeof(per_obj_cb_t));
  DX_CHECK_RETURN(m_cmd_allocators[m_frame_no]->Reset());
  DX_CHECK_RETURN(m_cmd_list->Reset(m_cmd_allocators[m_frame_no], m_shadow_pso));

  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = (float)m_width;
  viewport.Height = (float)m_height;
  viewport.MinDepth = D3D12_MIN_DEPTH;
  viewport.MaxDepth = D3D12_MAX_DEPTH;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0;
  scissor_rect.top = 0;
  scissor_rect.right = m_width;
  scissor_rect.bottom = m_height;
  m_cmd_list->RSSetViewports(1, &viewport);
  m_cmd_list->RSSetScissorRects(1, &scissor_rect);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

  m_cmd_list->SetPipelineState(m_shadow_pso);
  m_cmd_list->SetGraphicsRootSignature(m_shadow_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_shadow_shared_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(0, NULL, FALSE, &m_shadow_depth_rt_descriptor.cpu_handle);
  m_cmd_list->ClearDepthStencilView(m_shadow_depth_rt_descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_vertices_vb_view);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_per_obj_cbv_descriptors[0].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_nums[0], 1, 0, 0);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_per_obj_cbv_descriptors[1].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_nums[1], 1, m_obj_vertices_nums[0], 0);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

  m_cmd_list->SetPipelineState(m_final_pso);
  m_cmd_list->SetGraphicsRootSignature(m_final_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_shadow_srv_descriptor.gpu_handle);
  m_cmd_list->SetGraphicsRootDescriptorTable(2, m_final_shared_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(1, &m_rtv_descriptors[m_frame_no].cpu_handle, FALSE, &m_depth_rt_descriptor.cpu_handle);
  float clear_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
  m_cmd_list->ClearRenderTargetView(m_rtv_descriptors[m_frame_no].cpu_handle, clear_color, 0, NULL);
  m_cmd_list->ClearDepthStencilView(m_depth_rt_descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_vertices_vb_view);
  m_cmd_list->IASetVertexBuffers(1, 1, &m_normals_vb_view);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_per_obj_cbv_descriptors[0].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_nums[0], 1, 0, 0);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_per_obj_cbv_descriptors[1].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_nums[1], 1, m_obj_vertices_nums[0], 0);

  m_cmd_list->SetPipelineState(m_ui_pso);
  m_cmd_list->SetGraphicsRootSignature(m_ui_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_texture_srv_descriptor.gpu_handle);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_ui_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(1, &m_rtv_descriptors[m_frame_no].cpu_handle, FALSE, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_ui_vb_view);
  m_cmd_list->DrawInstanced(m_visible_text_len * 6, 1, 0, 0);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  m_cmd_list->Close();
  m_cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_cmd_list);
  DX_CHECK_RETURN(m_swap_chain->Present(1, 0));
  // Prepare to render the next frame
  U64 curr_fence_val = m_fence_vals[m_frame_no];
  DX_CHECK_RETURN(m_cmd_queue->Signal(m_fence, curr_fence_val));
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();
  // If the next frame is not ready to be rendered yet, wait until it's ready.
  if (m_fence->GetCompletedValue() < m_fence_vals[m_frame_no]) {
    DX_CHECK_RETURN(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
    WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
  }
  m_fence_vals[m_frame_no] = curr_fence_val + 1;
}

void DX12Window::on_mouse_event(EMouse mouse, int x, int y, bool is_down) {
  m_cam.cam_mouse_event(mouse, x, y, is_down);
}

void DX12Window::on_mouse_move(int x, int y) {
  m_cam.cam_mouse_move(x, y);
}

void DX12Window::on_key_event(EKey key, bool is_down) {
  if (key == EKEY_BELOW_ESC) {
    m_is_console_active = !m_is_console_active;
  }
}

void DX12Window::on_char_event(wchar_t c) {
  qc_append_codepoint((U32)c);
}

void DX12Window::wait_for_gpu() {
  // Wait for pending GPU work to complete.

  // Schedule a Signal command in the queue.
  DX_CHECK_RETURN(m_cmd_queue->Signal(m_fence, m_fence_vals[m_frame_no]));

  // Wait until the fence has been processed.
  DX_CHECK_RETURN(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
  WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);

  // Increment the fence value for the current frame.
  ++m_fence_vals[m_frame_no];
}

int main() {
  core_init(OS_TXT("dx12_sample.log"));
  DX12Window w(OS_TXT("dx12_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}

#if IS_CLANG()
#pragma clang diagnostic pop
#endif
