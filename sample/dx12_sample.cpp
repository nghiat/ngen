//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
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

#define M_dx_check_return_(condition) M_check_return(condition == S_OK)
#define M_dx_check_return_false_(condition) M_check_return_val(condition == S_OK, false)

#if M_compiler_is_clang()
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

enum E_enable_depth_ {
  e_enable_depth_false,
  e_enable_depth_true,
};

struct Code_point_ {
  V2 uv_top_left;
  V2 uv_bottom_right;
  int x_left, x_right, y_bottom, y_top;
  int advance;
  int lsb;
};

struct Font_ {
  stbtt_fontinfo fontinfo;
  Code_point_ codepoints[32*1024];
  int baseline;
  int line_space;
  float scale;
} g_font;

struct Per_obj_cb_ {
  M4 world;
};

struct Shadow_shared_cb_ {
  M4 light_view;
  M4 light_proj;
};

struct Final_shared_cb_ {
  M4 view;
  M4 proj;
  M4 light_view;
  M4 light_proj;
  V4 eye_pos;
  V4 obj_color;
  V4 light_pos;
  V4 light_color;
};

struct Ui_cb_t {
  V4 color;
  F32 width;
  F32 height;
};

struct Dx12_descriptor_heap {
  ID3D12DescriptorHeap* heap = NULL;
  U32 increment_size = 0;
  U32 curr_index = 0;
};

struct Dx12_descriptor {
  Dx12_descriptor_heap* descriptor_heap = NULL;
  U32 index = 0;
  D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle = {};
  D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle = {};
};

struct Dx12_buffer {
  ID3D12Resource* buffer = NULL;
  void* cpu_p = NULL;
  Sip offset = 0;
};

struct Dx12_subbuffer {
  Dx12_buffer* buffer = NULL;
  U8* cpu_p = NULL;
  D3D12_GPU_VIRTUAL_ADDRESS gpu_p = 0;
  Sip offset = 0;
  Sip size = 0;
};

class Dx12_window : public ngWindow {
public:
  Dx12_window(const Os_char* title, int w, int h) : ngWindow(title, w, h) {}

  bool init();
  void destroy() override;
  void loop() override;
  void on_mouse_event(E_mouse mouse, int x, int y, bool is_down) override;
  void on_mouse_move(int x, int y) override;
  void on_key_event(E_key key, bool is_down) override;
  void on_char_event(wchar_t c) override;

  void wait_for_gpu();

  ngCam m_cam;

  static const int sc_frame_count = 2;
  static D3D12_RASTERIZER_DESC s_default_rasterizer_desc;
  static D3D12_BLEND_DESC s_default_blend_desc;
  int m_frame_no;
  ID3D12Device* m_device = NULL;
  ID3D12CommandQueue* m_cmd_queue = NULL;
  IDXGISwapChain3* m_swap_chain = NULL;

  Dx12_descriptor_heap m_rtv_heap;
  ID3D12Resource* m_render_targets[sc_frame_count];
  Dx12_descriptor m_rtv_descriptors[sc_frame_count];

  ID3D12CommandAllocator* m_cmd_allocators[sc_frame_count];
  ID3D12GraphicsCommandList* m_cmd_list;

  Dx12_descriptor_heap m_cbv_srv_heap;
  Dx12_buffer m_upload_buffer;
  Dx12_descriptor m_shadow_srv_descriptor;

  Dx12_descriptor m_per_obj_cbv_descriptors[10];
  Dx12_subbuffer m_per_obj_cb_subbuffers[10];
  Per_obj_cb_ m_per_obj_cbs[10];

  Dx12_descriptor m_shadow_shared_cbv_descriptor;
  Dx12_subbuffer m_shadow_shared_cb_subbuffer;
  Shadow_shared_cb_ m_shadow_shared_cb;

  Dx12_descriptor m_final_shared_cbv_descriptor;
  Dx12_subbuffer m_final_shared_cb_subbuffer;
  Final_shared_cb_ m_final_shared_cb;

  Dx12_descriptor m_shared_ui_cbv_descriptor;
  Dx12_subbuffer m_shared_ui_cb_subbuffer;
  Ui_cb_t m_shared_ui_cb;

  Dx12_descriptor_heap m_dsv_heap;
  ID3D12Resource* m_depth_stencil;
  ID3D12Resource* m_shadow_depth_stencil;
  Dx12_descriptor m_depth_rt_descriptor;
  Dx12_descriptor m_shadow_depth_rt_descriptor;

  ID3D12RootSignature* m_shadow_root_sig;
  ID3D12RootSignature* m_final_root_sig;
  ID3D12RootSignature* m_ui_root_sig;
  ID3D12RootSignature* m_console_root_sig;
  ID3DBlob* m_final_vs;
  ID3DBlob* m_final_ps;
  ID3DBlob* m_shadow_vs;
  ID3DBlob* m_ui_texture_vs;
  ID3DBlob* m_ui_texture_ps;
  ID3DBlob* m_ui_non_texture_vs;
  ID3DBlob* m_ui_non_texture_ps;

  ID3D12PipelineState* m_shadow_pso;
  ID3D12PipelineState* m_final_pso;
  ID3D12PipelineState* m_ui_texture_pso;
  ID3D12PipelineState* m_console_pso;

  Dx12_buffer m_vertex_buffer;
  Dx12_subbuffer m_vertices_subbuffer;
  Dx12_subbuffer m_normals_subbuffer;
  Dx12_subbuffer m_text_subbuffer;
  Dx12_subbuffer m_console_subbuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertices_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_normals_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_text_vb_view;
  D3D12_VERTEX_BUFFER_VIEW m_console_vb_view;
  U32 m_obj_count = 0;
  U32 m_obj_vertices_counts[10] = {};
  Dx12_subbuffer m_texture_subbuffer;
  ID3D12Resource* m_texture;
  Dx12_descriptor m_texture_srv_descriptor;
  U32 m_visible_text_len = 0;

  const U32 m_normals_stride = 64 * 1024 * 1024;

  ID3D12Fence* m_fence;
  HANDLE m_fence_event;
  U64 m_fence_vals[sc_frame_count] = {};

  Quake_console m_console;

  S64 m_start_time;
  bool m_is_console_active = false;
private:
  void create_pso_(ID3D12PipelineState** pso,
                  ID3D12RootSignature* root_sig,
                  ID3DBlob* vs,
                  ID3DBlob* ps,
                  D3D12_INPUT_ELEMENT_DESC* element_desc,
                  int element_count,
                  E_enable_depth_ enable_depth = e_enable_depth_false);
  void create_root_sig_(ID3D12RootSignature** root_sig,
                     UINT root_param_count,
                     const D3D12_ROOT_PARAMETER1* root_params,
                     UINT static_sampler_count,
                     const D3D12_STATIC_SAMPLER_DESC* static_samplers,
                     D3D12_ROOT_SIGNATURE_FLAGS flags);
  void add_text_at_(const char* text, F32 x_left, F32 y_top, F32 wrap_width, Dx12_subbuffer* subbuffer, D3D12_VERTEX_BUFFER_VIEW* vb_view);
};

static bool create_descriptor_heap_(Dx12_descriptor_heap* descriptor_heap, ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, U32 max_descriptor_count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = type;
  desc.NumDescriptors = max_descriptor_count;
  desc.Flags = flags;
  desc.NodeMask = 0;
  M_dx_check_return_false_(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&descriptor_heap->heap)));
  descriptor_heap->increment_size = device->GetDescriptorHandleIncrementSize(type);
  return true;
}

static Dx12_descriptor allocate_descriptor_(Dx12_descriptor_heap* descriptor_heap) {
  Dx12_descriptor descriptor;
  M_check_log_return_val(descriptor_heap->curr_index < descriptor_heap->heap->GetDesc().NumDescriptors, descriptor, "Out of descriptors");
  descriptor.descriptor_heap = descriptor_heap;
  descriptor.index = descriptor_heap->curr_index;
  descriptor.cpu_handle.ptr = descriptor_heap->heap->GetCPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  descriptor.gpu_handle.ptr = descriptor_heap->heap->GetGPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  ++descriptor_heap->curr_index;
  return descriptor;
}

static D3D12_RESOURCE_DESC create_resource_desc_(size_t size) {
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

static D3D12_HEAP_PROPERTIES create_heap_props_(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES props = {};
  props.Type = type;
  props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  props.CreationNodeMask = 1;
  props.VisibleNodeMask = 1;
  return props;
}

static D3D12_DESCRIPTOR_RANGE1 create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE range_type,
                                                           UINT descriptor_count,
                                                           UINT base_shader_reg,
                                                           UINT reg_space,
                                                           D3D12_DESCRIPTOR_RANGE_FLAGS flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC,
                                                           UINT offset_in_descriptors_from_table_start = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND) {
  D3D12_DESCRIPTOR_RANGE1 range = {};
  range.RangeType = range_type;
  range.NumDescriptors = descriptor_count;
  range.BaseShaderRegister = base_shader_reg;
  range.RegisterSpace = reg_space;
  range.Flags = flags;
  range.OffsetInDescriptorsFromTableStart = offset_in_descriptors_from_table_start;
  return range;
}

static D3D12_ROOT_PARAMETER1
create_root_param_1_1_descriptor_table_(UINT range_count, const D3D12_DESCRIPTOR_RANGE1* ranges, D3D12_SHADER_VISIBILITY shader_visibility) {
  D3D12_ROOT_PARAMETER1 param = {};
  param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
  param.DescriptorTable = {};
  param.DescriptorTable.NumDescriptorRanges = range_count;
  param.DescriptorTable.pDescriptorRanges = ranges;
  param.ShaderVisibility = shader_visibility;
  return param;
}

static D3D12_RESOURCE_BARRIER create_transition_barrier_(ID3D12Resource* resource, D3D12_RESOURCE_STATES before, D3D12_RESOURCE_STATES after) {
  D3D12_RESOURCE_BARRIER barrier = {};
  barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  barrier.Transition.pResource = resource;
  barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
  barrier.Transition.StateBefore = before;
  barrier.Transition.StateAfter = after;
  return barrier;
}

static D3D12_CONSTANT_BUFFER_VIEW_DESC create_const_buf_view_desc_(D3D12_GPU_VIRTUAL_ADDRESS location, UINT size) {
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  desc.BufferLocation = location;
  desc.SizeInBytes = size;
  return desc;
}

static bool create_buffer_(Dx12_buffer* buffer, ID3D12Device* device, const D3D12_HEAP_PROPERTIES* heap_props, const D3D12_RESOURCE_DESC* desc) {
  *buffer = {};
  M_dx_check_return_false_(device->CreateCommittedResource(heap_props, D3D12_HEAP_FLAG_NONE, desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&buffer->buffer)));
  buffer->buffer->Map(0, NULL, &buffer->cpu_p);
  return true;
}

static Dx12_subbuffer allocate_subbuffer_(Dx12_buffer* buffer, Sip size, Sip alignment) {
  Dx12_subbuffer subbuffer = {};
  Sip aligned_offset = (buffer->offset + alignment - 1) & ~(alignment - 1);
  M_check_log_return_val(aligned_offset + size <= buffer->buffer->GetDesc().Width, subbuffer, "Out of memory");
  subbuffer.buffer = buffer;
  subbuffer.cpu_p = (U8*)buffer->cpu_p + aligned_offset;
  subbuffer.gpu_p = buffer->buffer->GetGPUVirtualAddress() + aligned_offset;
  subbuffer.offset = aligned_offset;
  subbuffer.size = size;
  buffer->offset = aligned_offset + size;
  return subbuffer;
}

static bool compile_shader_(const Os_char* path, const char* entry, const char* target, ID3DBlob** shader) {
  UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
  ID3DBlob* error;
  if (D3DCompileFromFile(path, NULL, NULL, entry, target, compile_flags, 0, shader, &error) != S_OK) {
    M_logf("%s", (const char*)error->GetBufferPointer());
    return false;
  }
  return true;
}

D3D12_RASTERIZER_DESC Dx12_window::s_default_rasterizer_desc = {
    .FillMode = D3D12_FILL_MODE_SOLID,
    .CullMode = D3D12_CULL_MODE_BACK,
    .FrontCounterClockwise = TRUE,
    .DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
    .DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
    .SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
    .DepthClipEnable = TRUE,
    .MultisampleEnable = FALSE,
    .AntialiasedLineEnable = FALSE,
    .ForcedSampleCount = 0,
    .ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,
};

D3D12_BLEND_DESC Dx12_window::s_default_blend_desc = {
    .AlphaToCoverageEnable = FALSE,
    .IndependentBlendEnable = FALSE,
};

bool Dx12_window::init() {
  ngWindow::init();

  m_start_time = mono_time_now();
  m_cam.init({5.0f, 5.0f, 5.0f}, {0.0f, 0.0f, 0.0f}, this);
  m_final_shared_cb.eye_pos = V3o_v4(m_cam.m_eye, 1.0f);
  m_final_shared_cb.obj_color = {1.0f, 0.0f, 0.0f, 1.0f};
  m_final_shared_cb.light_pos = {10.0f, 10.0f, 10.0f, 1.0f};
  m_final_shared_cb.light_color = {1.0f, 1.0f, 1.0f, 1.0f};

  // The light is static for now.
  ngCam light_cam;
  light_cam.init({1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, this);
  // TODO: ortho?
  M4 perspective_m4 = perspective(degree_to_rad(75), m_width * 1.0f / m_height, 0.01f, 100.0f);

  m_shadow_shared_cb.light_view = light_cam.m_view_mat;
  m_shadow_shared_cb.light_proj = perspective_m4;

  m_final_shared_cb.proj = perspective_m4;
  m_final_shared_cb.light_view = light_cam.m_view_mat;
  m_final_shared_cb.light_proj = perspective_m4;

  m_shared_ui_cb.color = {173 / 255.f, 216 / 255.f, 230 / 255.f, 0.2f};
  m_shared_ui_cb.width = (F32)m_width;
  m_shared_ui_cb.height = (F32)m_height;

  UINT dxgi_factory_flags = 0;
  {
    // Enable debug layer.
    ID3D12Debug* debug_controller;
    M_dx_check_return_false_(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)));
    debug_controller->EnableDebugLayer();
    dxgi_factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
    debug_controller->Release();
  }

  IDXGIFactory4* dxgi_factory;
  M_dx_check_return_false_(CreateDXGIFactory2(dxgi_factory_flags, IID_PPV_ARGS(&dxgi_factory)));

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
      M_check_log_return_val(backup_adapter_i != -1, false, "Can't find a dx12 adapter");
      adapter_i = backup_adapter_i;
    }

    dxgi_factory->EnumAdapters1(adapter_i, &adapter);
    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    M_logi("%ls", desc.Description);
    M_dx_check_return_false_(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));
  }

  {
    D3D12_COMMAND_QUEUE_DESC cmd_queue_desc = {};
    cmd_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    cmd_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    M_dx_check_return_false_(m_device->CreateCommandQueue(&cmd_queue_desc, IID_PPV_ARGS(&m_cmd_queue)));
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
  M_dx_check_return_false_(dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue, m_platform_data.hwnd, &sc_desc, NULL, NULL, &swap_chain));
  M_dx_check_return_false_(dxgi_factory->MakeWindowAssociation(m_platform_data.hwnd, DXGI_MWA_NO_ALT_ENTER));
  M_dx_check_return_false_(swap_chain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)));
  dxgi_factory->Release();
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();

  {
    if (!create_descriptor_heap_(&m_rtv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, sc_frame_count))
      return false;

    for (int i = 0; i < sc_frame_count; ++i) {
      M_dx_check_return_false_(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_render_targets[i])));
      m_rtv_descriptors[i] = allocate_descriptor_(&m_rtv_heap);
      m_device->CreateRenderTargetView(m_render_targets[i], NULL, m_rtv_descriptors[i].cpu_handle);
    }
  }

  {
    if (!create_descriptor_heap_(&m_dsv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2))
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

    D3D12_HEAP_PROPERTIES heap_props = create_heap_props_(D3D12_HEAP_TYPE_DEFAULT);

    M_dx_check_return_false_(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&m_depth_stencil)));
    m_depth_rt_descriptor = allocate_descriptor_(&m_dsv_heap);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_view_desc = {};
    dsv_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_view_desc.Texture2D.MipSlice = 0;
    m_device->CreateDepthStencilView(m_depth_stencil, &dsv_view_desc, m_depth_rt_descriptor.cpu_handle);

    M_dx_check_return_false_(m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_GENERIC_READ, &clear_value, IID_PPV_ARGS(&m_shadow_depth_stencil)));
    m_shadow_depth_rt_descriptor = allocate_descriptor_(&m_dsv_heap);
    m_device->CreateDepthStencilView(m_shadow_depth_stencil, &dsv_view_desc, m_shadow_depth_rt_descriptor.cpu_handle);
  }

  {
    if(!create_descriptor_heap_(&m_cbv_srv_heap, m_device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10))
      return false;

    {
      m_shadow_srv_descriptor = allocate_descriptor_(&m_cbv_srv_heap);
      // Allocate here and use later so they will be next to each other.
      m_texture_srv_descriptor = allocate_descriptor_(&m_cbv_srv_heap);
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
    D3D12_RESOURCE_DESC upload_desc = create_resource_desc_(64 * 1024 * 1024);
    D3D12_HEAP_PROPERTIES heap_props = create_heap_props_(D3D12_HEAP_TYPE_UPLOAD);
    if (!create_buffer_(&m_upload_buffer, m_device, &heap_props, &upload_desc))
      return false;
    // From D3D12HelloConstantBuffers sample
    // CB size is required to be 256-byte aligned
    {
      m_shadow_shared_cbv_descriptor = allocate_descriptor_(&m_cbv_srv_heap);
      Sip cb_size = (sizeof(Shadow_shared_cb_) + 255) & ~255;
      m_shadow_shared_cb_subbuffer = allocate_subbuffer_(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc_(m_shadow_shared_cb_subbuffer.gpu_p, m_shadow_shared_cb_subbuffer.size), m_shadow_shared_cbv_descriptor.cpu_handle);
      memcpy(m_shadow_shared_cb_subbuffer.cpu_p, &m_shadow_shared_cb, sizeof(m_shadow_shared_cb));
    }

    {
      m_final_shared_cbv_descriptor = allocate_descriptor_(&m_cbv_srv_heap);
      Sip cb_size = (sizeof(Final_shared_cb_) + 255) & ~255;
      m_final_shared_cb_subbuffer = allocate_subbuffer_(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc_(m_final_shared_cb_subbuffer.gpu_p, m_final_shared_cb_subbuffer.size), m_final_shared_cbv_descriptor.cpu_handle);
    }

    {
      m_shared_ui_cbv_descriptor = allocate_descriptor_(&m_cbv_srv_heap);
      Sip cb_size = (sizeof(Ui_cb_t) + 255) & ~255;
      m_shared_ui_cb_subbuffer = allocate_subbuffer_(&m_upload_buffer, cb_size, 256);
      m_device->CreateConstantBufferView(&create_const_buf_view_desc_(m_shared_ui_cb_subbuffer.gpu_p, m_shared_ui_cb_subbuffer.size), m_shared_ui_cbv_descriptor.cpu_handle);
      memcpy(m_shared_ui_cb_subbuffer.cpu_p, &m_shared_ui_cb, sizeof(m_shared_ui_cb));
    }
  }

  {
    // Create a root signature consisting of a descriptor table with a single CBV
    D3D12_DESCRIPTOR_RANGE1 ranges[] = {
      create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
      create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0),
    };
    D3D12_ROOT_PARAMETER1 root_params[] = {
      create_root_param_1_1_descriptor_table_(1, &ranges[0], D3D12_SHADER_VISIBILITY_VERTEX),
      create_root_param_1_1_descriptor_table_(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX),
    };
    create_root_sig_(&m_shadow_root_sig,
                    static_array_size(root_params),
                    root_params,
                    0,
                    NULL,
                    D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
                        D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS);
  }

  {
    Os_char shader_path[M_max_path_len];
    path_from_exe_dir(shader_path, M_os_txt("assets/shadow.hlsl"), M_max_path_len);
    compile_shader_(shader_path, "VSMain", "vs_5_0", &m_shadow_vs);

    path_from_exe_dir(shader_path, M_os_txt("assets/shader.hlsl"), M_max_path_len);
    compile_shader_(shader_path, "VSMain", "vs_5_0", &m_final_vs);
    compile_shader_(shader_path, "PSMain", "ps_5_0", &m_final_ps);

    path_from_exe_dir(shader_path, M_os_txt("assets/ui.hlsl"), M_max_path_len);
    compile_shader_(shader_path, "VSTextureMain", "vs_5_0", &m_ui_texture_vs);
    compile_shader_(shader_path, "PSTextureMain", "ps_5_0", &m_ui_texture_ps);
    compile_shader_(shader_path, "VSNonTextureMain", "vs_5_0", &m_ui_non_texture_vs);
    compile_shader_(shader_path, "PSNonTextureMain", "ps_5_0", &m_ui_non_texture_ps);
  }

  {
    for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
      D3D12_RENDER_TARGET_BLEND_DESC* rt_desc = &s_default_blend_desc.RenderTarget[i];
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

    {
      D3D12_INPUT_ELEMENT_DESC input_elem_descs[] = {
        { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      };
      create_pso_(&m_shadow_pso, m_shadow_root_sig, m_shadow_vs, NULL, input_elem_descs, static_array_size(input_elem_descs), e_enable_depth_true);
    }

    for (int i = 0; i < sc_frame_count; ++i)
      M_dx_check_return_false_(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocators[i])));
    M_dx_check_return_false_(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocators[m_frame_no], m_shadow_pso, IID_PPV_ARGS(&m_cmd_list)));

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table_(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL),
          create_root_param_1_1_descriptor_table_(1, &ranges[1], D3D12_SHADER_VISIBILITY_ALL),
          create_root_param_1_1_descriptor_table_(1, &ranges[2], D3D12_SHADER_VISIBILITY_ALL),
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

      create_root_sig_(&m_final_root_sig, static_array_size(root_params), root_params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    {
      D3D12_INPUT_ELEMENT_DESC final_elem_descs[] = {
        { "V", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "N", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      };
      create_pso_(&m_final_pso, m_final_root_sig, m_final_vs, m_final_ps, final_elem_descs, static_array_size(final_elem_descs), e_enable_depth_true);
    }

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table_(1, &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL),
          create_root_param_1_1_descriptor_table_(1, &ranges[1], D3D12_SHADER_VISIBILITY_VERTEX),
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

      create_root_sig_(&m_ui_root_sig, static_array_size(root_params), root_params, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    {
      D3D12_DESCRIPTOR_RANGE1 ranges[] = {
        create_descriptor_range_1_1_(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE),
      };
      D3D12_ROOT_PARAMETER1 root_params[] = {
          create_root_param_1_1_descriptor_table_(1, &ranges[0], D3D12_SHADER_VISIBILITY_ALL),
      };

      create_root_sig_(&m_console_root_sig, static_array_size(root_params), root_params, 0, NULL, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    }

    {
      D3D12_INPUT_ELEMENT_DESC ui_elem_descs[] = {
        { "V", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(F32), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      };
      create_pso_(&m_ui_texture_pso, m_ui_root_sig, m_ui_texture_vs, m_ui_texture_ps, ui_elem_descs, static_array_size(ui_elem_descs));
    }

    {
      D3D12_INPUT_ELEMENT_DESC console_elem_descs[] = {
        { "V", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      };
      create_pso_(&m_console_pso, m_console_root_sig, m_ui_non_texture_vs, m_ui_non_texture_ps, console_elem_descs, static_array_size(console_elem_descs));
    }
  }

  {
    stbtt_fontinfo font;
    Os_char font_path[M_max_path_len];
    path_from_exe_dir(font_path, M_os_txt("assets/UbuntuMono-Regular.ttf"), M_max_path_len);
    Dynamic_array<U8> font_buf = File::read_whole_file_as_text(g_persistent_allocator, font_path);
    M_check_return_val(stbtt_InitFont(&font, &font_buf[0], stbtt_GetFontOffsetForIndex(&font_buf[0], 0)) != 0, false);
    g_font.fontinfo = font;
    const int c_tex_w = 2048;
    const int c_tex_h = 2048;
    Linear_allocator<> temp_allocator("font_allocator");
    temp_allocator.init();
    M_scope_exit(temp_allocator.destroy());
    U8* Font_ex = (U8*)temp_allocator.alloc_zero(c_tex_w * c_tex_h);
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
      int x_left, y_bottom, x_right, y_top;
      stbtt_GetCodepointBitmapBoxSubpixel(&font, c, scale, scale, x_shift, 0.0f, &x_left, &y_bottom, &x_right, &y_top);
      int w = x_right - x_left;
      int h = y_top - y_bottom;
      M_check_log_return_val(y + h < c_tex_h, false, "Bigger than texture height");
      if (x + w + 1 >= c_tex_w) {
        y += font_h;
        x = c_x_left;
      }
      // Is +1 necessary for subpixel rendering?
      M_check_log_return_val(x + w + 1 < c_tex_h, false, "Bigger than texture width");
      stbtt_MakeCodepointBitmapSubpixel(&font, &Font_ex[(int)(y) * c_tex_w + (int)x], w, h, c_tex_w, scale, scale, x_shift, 0.0f, c);
      int advance, lsb;
      stbtt_GetCodepointHMetrics(&font, c, &advance, &lsb);
      g_font.codepoints[c].uv_top_left = V2{x + 0.5f, y + 0.5f} / V2{c_tex_w, c_tex_h};
      g_font.codepoints[c].uv_bottom_right = V2{x + w - 0.5f, y + h - 0.5f} / V2{c_tex_w, c_tex_h};
      g_font.codepoints[c].advance = advance;
      g_font.codepoints[c].lsb = lsb;
      g_font.codepoints[c].x_left = x_left;
      g_font.codepoints[c].x_right = x_right;
      g_font.codepoints[c].y_bottom = y_bottom;
      g_font.codepoints[c].y_top = y_top;
      x += w;
    }

    D3D12_SUBRESOURCE_FOOTPRINT pitched_desc = {};
    pitched_desc.Format = DXGI_FORMAT_R8_UNORM;
    pitched_desc.Width = c_tex_w;
    pitched_desc.Height = c_tex_h;
    pitched_desc.Depth = 1;
    pitched_desc.RowPitch = (c_tex_w + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
    m_texture_subbuffer = allocate_subbuffer_(&m_upload_buffer, pitched_desc.Height * pitched_desc.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_texture = {};
    placed_texture.Offset = (Sip)m_texture_subbuffer.cpu_p - (Sip)m_texture_subbuffer.buffer->cpu_p;
    placed_texture.Footprint = pitched_desc;
    for (int i = 0; i < pitched_desc.Height; ++i) {
      memcpy(m_texture_subbuffer.cpu_p + i * pitched_desc.RowPitch, &Font_ex[i * c_tex_w], pitched_desc.Width);
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
    M_dx_check_return_false_(m_device->CreateCommittedResource(&create_heap_props_(D3D12_HEAP_TYPE_DEFAULT),
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
    m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(m_texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
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
    Linear_allocator<> temp_allocator("obj_allocator");
    temp_allocator.init();
    M_scope_exit(temp_allocator.destroy());
    const Os_char* obj_paths[] = {
        M_os_txt("assets/plane.obj"),
        M_os_txt("assets/wolf.obj"),
    };
    Sip vertices_offset = 0;
    Sip normals_offset = 0;
    m_obj_count = static_array_size(obj_paths);

    {
      Sip cb_size = (sizeof(Per_obj_cb_) + 255) & ~255;
      for (int i = 0; i < m_obj_count; ++i) {
        m_per_obj_cb_subbuffers[i] = allocate_subbuffer_(&m_upload_buffer, cb_size, 256);
        m_per_obj_cbv_descriptors[i] = allocate_descriptor_(&m_cbv_srv_heap);
        m_device->CreateConstantBufferView(&create_const_buf_view_desc_(m_per_obj_cb_subbuffers[i].gpu_p, cb_size), m_per_obj_cbv_descriptors[i].cpu_handle);
        m_per_obj_cbs[i].world = m4_identity();
        memcpy(m_per_obj_cb_subbuffers[i].cpu_p, &m_per_obj_cbs[i], sizeof(Per_obj_cb_));
      }
    }

    if (!create_buffer_(&m_vertex_buffer, m_device, &create_heap_props_(D3D12_HEAP_TYPE_UPLOAD), &create_resource_desc_(128 * 1024 * 1024)))
      return false;

    m_vertices_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 16 * 1024 * 1024, 16);
    m_normals_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 16 * 1024 * 1024, 16);
    m_text_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 1024 * 1024, 16);
    m_console_subbuffer = allocate_subbuffer_(&m_vertex_buffer, 6 * sizeof(V2), 16);
    for (int i = 0; i < m_obj_count; ++i) {
      Obj_loader obj;
      Os_char full_obj_path[M_max_path_len];
      obj.init(&temp_allocator, path_from_exe_dir(full_obj_path, obj_paths[i], M_max_path_len));
      m_obj_vertices_counts[i] = obj.m_vertices.len();
      int vertices_size = m_obj_vertices_counts[i] * sizeof(obj.m_vertices[0]);
      int normals_size = m_obj_vertices_counts[i] * sizeof(obj.m_normals[0]);
      M_check_return_val(vertices_offset + vertices_size <= m_vertices_subbuffer.size, false);
      M_check_return_val(normals_offset + normals_size <= m_normals_subbuffer.size, false);
      memcpy(m_vertices_subbuffer.cpu_p + vertices_offset, &obj.m_vertices[0], vertices_size);
      memcpy(m_normals_subbuffer.cpu_p + normals_offset, &obj.m_normals[0], normals_size);
      vertices_offset += vertices_size;
      normals_offset += normals_size;
    }
    m_vertices_vb_view.BufferLocation = m_vertices_subbuffer.gpu_p;
    m_vertices_vb_view.SizeInBytes = vertices_offset;
    m_vertices_vb_view.StrideInBytes = sizeof(((Obj_loader*)0)->m_vertices[0]);

    m_normals_vb_view.BufferLocation = m_normals_subbuffer.gpu_p;
    m_normals_vb_view.SizeInBytes = normals_offset;
    m_normals_vb_view.StrideInBytes = sizeof(((Obj_loader*)0)->m_normals[0]);

  }

  {
    m_console.init(m_width, 300.0f);
    memcpy(m_console_subbuffer.cpu_p, &m_console.m_rect[0], sizeof(m_console.m_rect));
    m_console_vb_view.BufferLocation = m_console_subbuffer.gpu_p;
    m_console_vb_view.SizeInBytes = 6 * sizeof(V2);
    m_console_vb_view.StrideInBytes = sizeof(V2);
  }

  m_vertex_buffer.buffer->Unmap(0, NULL);
  {
    M_dx_check_return_false_(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_vals[m_frame_no];
    m_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    M_check_return_val(m_fence_event, false);
    wait_for_gpu();
  }

  return true;
}

void Dx12_window::destroy() {
  if (m_device)
    m_device->Release();
  if (m_cmd_queue)
    m_cmd_queue->Release();
}

void Dx12_window::loop() {
  {
    add_text_at_((const char*)m_console.m_f_buf.m_p, 0.0f, 0.0f, m_console.m_width, &m_text_subbuffer, &m_text_vb_view);
  }

  m_cam.update();
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
  memcpy(m_per_obj_cb_subbuffers[1].cpu_p, &m_per_obj_cbs[1], sizeof(Per_obj_cb_));
  M_dx_check_return_(m_cmd_allocators[m_frame_no]->Reset());
  M_dx_check_return_(m_cmd_list->Reset(m_cmd_allocators[m_frame_no], m_shadow_pso));

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

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE));

  m_cmd_list->SetPipelineState(m_shadow_pso);
  m_cmd_list->SetGraphicsRootSignature(m_shadow_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_shadow_shared_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(0, NULL, FALSE, &m_shadow_depth_rt_descriptor.cpu_handle);
  m_cmd_list->ClearDepthStencilView(m_shadow_depth_rt_descriptor.cpu_handle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_vertices_vb_view);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_per_obj_cbv_descriptors[0].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_counts[0], 1, 0, 0);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_per_obj_cbv_descriptors[1].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_counts[1], 1, m_obj_vertices_counts[0], 0);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(m_shadow_depth_stencil, D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ));
  m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

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
  m_cmd_list->DrawInstanced(m_obj_vertices_counts[0], 1, 0, 0);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_per_obj_cbv_descriptors[1].gpu_handle);
  m_cmd_list->DrawInstanced(m_obj_vertices_counts[1], 1, m_obj_vertices_counts[0], 0);

  m_cmd_list->SetPipelineState(m_console_pso);
  m_cmd_list->SetGraphicsRootSignature(m_console_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_shared_ui_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(1, &m_rtv_descriptors[m_frame_no].cpu_handle, FALSE, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_console_vb_view);
  m_cmd_list->DrawInstanced(6, 1, 0, 0);

  m_cmd_list->SetPipelineState(m_ui_texture_pso);
  m_cmd_list->SetGraphicsRootSignature(m_ui_root_sig);
  m_cmd_list->SetDescriptorHeaps(1, &m_cbv_srv_heap.heap);
  m_cmd_list->SetGraphicsRootDescriptorTable(0, m_texture_srv_descriptor.gpu_handle);
  m_cmd_list->SetGraphicsRootDescriptorTable(1, m_shared_ui_cbv_descriptor.gpu_handle);
  m_cmd_list->OMSetRenderTargets(1, &m_rtv_descriptors[m_frame_no].cpu_handle, FALSE, NULL);
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->IASetVertexBuffers(0, 1, &m_text_vb_view);
  m_cmd_list->DrawInstanced(m_visible_text_len * 6, 1, 0, 0);

  m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(m_render_targets[m_frame_no], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

  m_cmd_list->Close();
  m_cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_cmd_list);
  M_dx_check_return_(m_swap_chain->Present(1, 0));
  // Prepare to render the next frame
  U64 curr_fence_val = m_fence_vals[m_frame_no];
  M_dx_check_return_(m_cmd_queue->Signal(m_fence, curr_fence_val));
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();
  // If the next frame is not ready to be rendered yet, wait until it's ready.
  if (m_fence->GetCompletedValue() < m_fence_vals[m_frame_no]) {
    M_dx_check_return_(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
    WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);
  }
  m_fence_vals[m_frame_no] = curr_fence_val + 1;
}

void Dx12_window::on_mouse_event(E_mouse mouse, int x, int y, bool is_down) {
  m_cam.mouse_event(mouse, x, y, is_down);
}

void Dx12_window::on_mouse_move(int x, int y) {
  m_cam.mouse_move(x, y);
}

void Dx12_window::on_key_event(E_key key, bool is_down) {
  if (key == e_key_below_esc) {
    m_is_console_active = !m_is_console_active;
  }
}

void Dx12_window::on_char_event(wchar_t c) {
  // append_codepoint((U32)c);
}

void Dx12_window::wait_for_gpu() {
  // Wait for pending GPU work to complete.

  // Schedule a Signal command in the queue.
  M_dx_check_return_(m_cmd_queue->Signal(m_fence, m_fence_vals[m_frame_no]));

  // Wait until the fence has been processed.
  M_dx_check_return_(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
  WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);

  // Increment the fence value for the current frame.
  ++m_fence_vals[m_frame_no];
}

void Dx12_window::create_pso_(ID3D12PipelineState** pso, ID3D12RootSignature* root_sig, ID3DBlob* vs, ID3DBlob* ps, D3D12_INPUT_ELEMENT_DESC* element_desc, int element_count, E_enable_depth_ enable_depth) {
  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.pRootSignature = root_sig;
  pso_desc.VS.pShaderBytecode = vs->GetBufferPointer();
  pso_desc.VS.BytecodeLength = vs->GetBufferSize();
  if (ps) {
    pso_desc.PS.pShaderBytecode = ps->GetBufferPointer();
    pso_desc.PS.BytecodeLength = ps->GetBufferSize();
  }
  pso_desc.BlendState = s_default_blend_desc;
  pso_desc.SampleMask = UINT_MAX;
  pso_desc.RasterizerState = s_default_rasterizer_desc;
  pso_desc.DepthStencilState = {};
  pso_desc.InputLayout.pInputElementDescs = element_desc;
  pso_desc.InputLayout.NumElements = element_count;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  pso_desc.SampleDesc.Count = 1;
  if (enable_depth == e_enable_depth_true) {
      pso_desc.DepthStencilState.DepthEnable = TRUE;
      pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
      pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
      pso_desc.DepthStencilState.StencilEnable = FALSE;
  }
  M_dx_check_return_(m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(pso)));
}

void Dx12_window::create_root_sig_(ID3D12RootSignature** root_sig,
                                 UINT root_param_count,
                                 const D3D12_ROOT_PARAMETER1* root_params,
                                 UINT static_sampler_count,
                                 const D3D12_STATIC_SAMPLER_DESC* static_samplers,
                                 D3D12_ROOT_SIGNATURE_FLAGS flags) {
  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
  desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  desc.Desc_1_1.NumParameters = root_param_count;
  desc.Desc_1_1.pParameters = root_params;
  desc.Desc_1_1.NumStaticSamplers = static_sampler_count;
  desc.Desc_1_1.pStaticSamplers = static_samplers;
  desc.Desc_1_1.Flags = flags;
  ID3DBlob* signature;
  ID3DBlob* error;
  M_dx_check_return_(D3D12SerializeVersionedRootSignature(&desc, &signature, &error));
  M_dx_check_return_(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(root_sig)));
}

void Dx12_window::add_text_at_(const char* text, F32 x_left, F32 y_top, F32 wrap_width, Dx12_subbuffer* subbuffer, D3D12_VERTEX_BUFFER_VIEW* vb_view) {
  Linear_allocator<> temp_allocator("text_allocator");
  temp_allocator.init();
  M_scope_exit(temp_allocator.destroy());
  m_vertex_buffer.buffer->Map(0, NULL, &m_vertex_buffer.cpu_p);
  Dynamic_array<float> ui_data;
  ui_data.init(&temp_allocator);
  float x = x_left;
  float y = y_top;
  int text_len = strlen(text);
  m_visible_text_len = 0;
  float scale = g_font.scale;
  int line_count = 0;
  for (int i = 0; i < text_len; ++i) {
    const char* c = &text[i];
    if (*c == '\n') {
      if (++line_count > 10) {
        break;
      }
      y += g_font.line_space;
      x = x_left;
      continue;
    }
    ++m_visible_text_len;
    const char* nc = &text[i + 1];
    Code_point_* cp = &g_font.codepoints[*c];
    float left = x + cp->x_left;
    float right = x + cp->x_right;
    if (right > wrap_width) {
      y += g_font.line_space;
      x = x_left;
      left = x + cp->x_left;
      right = x + cp->x_right;
    }
    float top = y - g_font.baseline + cp->y_bottom;
    float bottom = y - g_font.baseline + cp->y_top;
    ui_data.append(left);
    ui_data.append(top);
    ui_data.append(cp->uv_top_left.x);
    ui_data.append(cp->uv_top_left.y);

    ui_data.append(left);
    ui_data.append(bottom);
    ui_data.append(cp->uv_top_left.x);
    ui_data.append(cp->uv_bottom_right.y);

    ui_data.append(right);
    ui_data.append(top);
    ui_data.append(cp->uv_bottom_right.x);
    ui_data.append(cp->uv_top_left.y);

    ui_data.append(right);
    ui_data.append(top);
    ui_data.append(cp->uv_bottom_right.x);
    ui_data.append(cp->uv_top_left.y);

    ui_data.append(left);
    ui_data.append(bottom);
    ui_data.append(cp->uv_top_left.x);
    ui_data.append(cp->uv_bottom_right.y);

    ui_data.append(right);
    ui_data.append(bottom);
    ui_data.append(cp->uv_bottom_right.x);
    ui_data.append(cp->uv_bottom_right.y);

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
      if (x + word_length > wrap_width) {
        y += g_font.line_space;
        x = x_left;
      }
    }
  }
  memcpy((U8*)m_vertex_buffer.cpu_p + subbuffer->offset, &ui_data[0], ui_data.len() * sizeof(float));
  vb_view->BufferLocation = m_vertex_buffer.buffer->GetGPUVirtualAddress() + subbuffer->offset;
  vb_view->SizeInBytes = ui_data.len() * sizeof(float);
  vb_view->StrideInBytes = 4 * sizeof(float);
  m_vertex_buffer.buffer->Unmap(0, NULL);
}

int main() {
  core_init(M_os_txt("dx12_sample.log"));
  Dx12_window w(M_os_txt("dx12_sample"), 1024, 768);
  w.init();
  w.os_loop();
  return 0;
}

#if M_compiler_is_clang()
#pragma clang diagnostic pop
#endif
