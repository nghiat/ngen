//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/gpu/d3d12/d3d12.h"

#include "core/dynamic_array.inl"
#include "core/fixed_array.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/utils.h"

#include <d3dcompiler.h>

#define M_dx_check_return_(condition) {\
  HRESULT result_ = condition; \
  M_check_return(result_ == S_OK); \
}

#define M_dx_check_return_false_(condition) {\
  HRESULT result_ = condition; \
  M_check_return_val(result_ == S_OK, false); \
}

#define M_dx_check_return_val_(condition, val) {\
  HRESULT result_ = condition; \
  M_check_return_val(result_ == S_OK, val); \
}

#if M_compiler_is_clang()
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Waddress-of-temporary"
#endif

struct D3d12_texture_t : Texture_t {
  ID3D12Resource* texture;
};

struct D3d12_image_view_t : Image_view_t {
  D3d12_descriptor_t_ descriptor;
};

struct D3d12_resources_set_t : Resources_set_t {
  U8 uniform_buffer_count;
  U8 sampler_count;
  U8 image_count;
  E_shader_stage visibility;
};

struct D3d12_render_pass_t : Render_pass_t {
};

struct D3d12_pipeline_layout_t : Pipeline_layout_t {
  ID3D12RootSignature* root_signature;
};

struct D3d12_pipeline_state_object_t : Pipeline_state_object_t {
  ID3D12PipelineState* pso = NULL;
  ID3D12RootSignature* root_signature;
};

struct D3d12_sub_buffer_t_ {
  D3d12_buffer_t_* buffer = NULL;
  U8* cpu_p = NULL;
  D3D12_GPU_VIRTUAL_ADDRESS gpu_p = 0;
  Sip offset = 0;
  Sip size = 0;
};

struct D3d12_sampler_t : Sampler_t {
  D3d12_descriptor_t_ descriptor;
};

struct D3d12_uniform_buffer_t : Uniform_buffer_t {
  D3d12_sub_buffer_t_ sub_buffer;
  D3d12_descriptor_t_ descriptor;
};

struct D3d12_vertex_buffer_t : Vertex_buffer_t {
  D3d12_sub_buffer_t_ sub_buffer;
};

struct D3d12_shader_t : Shader_t {
  ID3DBlob* blob;
};

static D3d12_sub_buffer_t_ allocate_sub_buffer_(D3d12_buffer_t_* buffer, Sip size, Sip alignment) {
  D3d12_sub_buffer_t_ sub_buffer = {};
  Sip aligned_offset = (buffer->offset + alignment - 1) & ~(alignment - 1);
  size = (((size - 1) / 256) + 1) * 256;
  M_check_log_return_val(aligned_offset + size <= buffer->buffer->GetDesc().Width, sub_buffer, "Out of memory");
  sub_buffer.buffer = buffer;
  sub_buffer.cpu_p = (U8*)buffer->cpu_p + aligned_offset;
  sub_buffer.gpu_p = buffer->buffer->GetGPUVirtualAddress() + aligned_offset;
  sub_buffer.offset = aligned_offset;
  sub_buffer.size = size;
  buffer->offset = aligned_offset + size;
  return sub_buffer;
}

static D3d12_descriptor_t_ allocate_descriptor_(D3d12_descriptor_heap_t_* descriptor_heap) {
  D3d12_descriptor_t_ descriptor;
  M_check_log_return_val(descriptor_heap->curr_index < descriptor_heap->heap->GetDesc().NumDescriptors, descriptor, "Out of descriptors");
  descriptor.descriptor_heap = descriptor_heap;
  descriptor.index = descriptor_heap->curr_index;
  descriptor.cpu_handle.ptr = descriptor_heap->heap->GetCPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  descriptor.gpu_handle.ptr = descriptor_heap->heap->GetGPUDescriptorHandleForHeapStart().ptr + descriptor_heap->curr_index * descriptor_heap->increment_size;
  ++descriptor_heap->curr_index;
  return descriptor;
}

static DXGI_FORMAT convert_format_to_dxgi_format(E_format format) {
  switch(format) {
    case e_format_r32g32b32a32_float:
      return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case e_format_r32g32_float:
      return DXGI_FORMAT_R32G32_FLOAT;
    case e_format_r8_uint:
      return DXGI_FORMAT_R8_UINT;
    case e_format_r8_unorm:
      return DXGI_FORMAT_R8_UNORM;
    case e_format_r8g8b8a8_uint:
      return DXGI_FORMAT_R8G8B8A8_UINT;
    case e_format_r24_unorm_x8_typeless:
      return DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    default:
      M_unimplemented();
  }
  return DXGI_FORMAT_UNKNOWN;
}

static D3D12_RESOURCE_STATES convert_resource_state_to_d3d12_resource_state(E_resource_state state) {
  switch(state) {
    case e_resource_state_undefined:
      return D3D12_RESOURCE_STATE_COMMON;
    case e_resource_state_render_target:
      return D3D12_RESOURCE_STATE_RENDER_TARGET;
    case e_resource_state_depth_write:
      return D3D12_RESOURCE_STATE_DEPTH_WRITE;
    case e_resource_state_depth_read:
      return D3D12_RESOURCE_STATE_DEPTH_READ;
    case e_resource_state_present:
      return D3D12_RESOURCE_STATE_PRESENT;
    case e_resource_state_pixel_shader_resource:
      return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    default:
      M_unimplemented();
  }
  return D3D12_RESOURCE_STATE_COMMON;
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

static D3D12_HEAP_PROPERTIES create_heap_props_(D3D12_HEAP_TYPE type) {
  D3D12_HEAP_PROPERTIES props = {};
  props.Type = type;
  props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
  props.CreationNodeMask = 1;
  props.VisibleNodeMask = 1;
  return props;
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

static D3D12_SHADER_VISIBILITY convert_shader_stage_to_visibility(E_shader_stage visibility) {
  if (visibility & (visibility - 1)) {
    // more than one bit is set
    return D3D12_SHADER_VISIBILITY_ALL;
  }
  switch(visibility) {
    case e_shader_stage_vertex:
      return D3D12_SHADER_VISIBILITY_VERTEX;
    case e_shader_stage_fragment:
      return D3D12_SHADER_VISIBILITY_PIXEL;
    default:
      M_logf("Invalid visibility flag");
  }
  return (D3D12_SHADER_VISIBILITY)0;
}

static void update_root_signature_flags(D3D12_ROOT_SIGNATURE_FLAGS* flags, E_shader_stage visibility) {
  if (visibility & e_shader_stage_vertex) {
    *flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_VERTEX_SHADER_ROOT_ACCESS;
  }
  if (visibility & e_shader_stage_fragment) {
    *flags &= ~D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
  }
}

bool D3d12_t::init(Window_t* w) {
  m_window = w;
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
  sc_desc.Width = m_window->m_width;
  sc_desc.Height = m_window->m_height;
  sc_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sc_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sc_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  sc_desc.SampleDesc.Count = 1;

  IDXGISwapChain1* swap_chain;
  M_dx_check_return_false_(dxgi_factory->CreateSwapChainForHwnd(m_cmd_queue, m_window->m_platform_data.hwnd, &sc_desc, NULL, NULL, &swap_chain));
  M_dx_check_return_false_(dxgi_factory->MakeWindowAssociation(m_window->m_platform_data.hwnd, DXGI_MWA_NO_ALT_ENTER));
  M_dx_check_return_false_(swap_chain->QueryInterface(IID_PPV_ARGS(&m_swap_chain)));
  dxgi_factory->Release();
  m_frame_no = m_swap_chain->GetCurrentBackBufferIndex();

  {
    create_descriptor_heap_(&m_rtv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, sc_frame_count);
    for (int i = 0; i < sc_frame_count; ++i) {
      D3d12_render_target_t& rt = m_swapchain_rts[i];
      M_dx_check_return_false_(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&rt.resource)));
      rt.rtv_descriptor = allocate_descriptor_(&m_rtv_heap);
      m_device->CreateRenderTargetView(rt.resource, NULL, rt.rtv_descriptor.cpu_handle);
      rt.state = e_resource_state_present;
    }
  }
  create_descriptor_heap_(&m_dsv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 2);
  create_descriptor_heap_(&m_cbv_srv_heap, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10);
  create_descriptor_heap_(&m_sampler_heap, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10);

  m_device->CreateCommittedResource(&create_heap_props_(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &create_resource_desc_(64 * 1024 * 1024),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    NULL,
                                    IID_PPV_ARGS(&m_uniform_buffer.buffer));
  m_uniform_buffer.buffer->Map(0, NULL, &m_uniform_buffer.cpu_p);
  m_device->CreateCommittedResource(&create_heap_props_(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &create_resource_desc_(128 * 1024 * 1024),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    NULL,
                                    IID_PPV_ARGS(&m_vertex_buffer.buffer));
  m_vertex_buffer.buffer->Map(0, NULL, &m_vertex_buffer.cpu_p);
  m_device->CreateCommittedResource(&create_heap_props_(D3D12_HEAP_TYPE_UPLOAD),
                                    D3D12_HEAP_FLAG_NONE,
                                    &create_resource_desc_(128 * 1024 * 1024),
                                    D3D12_RESOURCE_STATE_GENERIC_READ,
                                    NULL,
                                    IID_PPV_ARGS(&m_upload_buffer.buffer));
  m_upload_buffer.buffer->Map(0, NULL, &m_upload_buffer.cpu_p);
  for (int i = 0; i < sc_frame_count; ++i) {
    M_dx_check_return_false_(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_cmd_allocators[i])));
  }
  M_dx_check_return_false_(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_cmd_allocators[m_frame_no], NULL, IID_PPV_ARGS(&m_cmd_list)));
  m_cmd_list->Close();
  {
    M_dx_check_return_false_(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    ++m_fence_vals[m_frame_no];
    m_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
    M_check_return_val(m_fence_event, false);
    wait_for_gpu_();
  }
  return true;
}

void D3d12_t::destroy() {
}

Texture_t* D3d12_t::create_texture(Allocator_t* allocator, const Texture_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_texture_t>();
  D3D12_RESOURCE_DESC texture_desc = {};
  texture_desc.MipLevels = 1;
  texture_desc.Format = convert_format_to_dxgi_format(ci.format);
  texture_desc.Width = ci.width;
  texture_desc.Height = ci.height;
  texture_desc.Flags = D3D12_RESOURCE_FLAG_NONE;
  texture_desc.DepthOrArraySize = 1;
  texture_desc.SampleDesc.Count = 1;
  texture_desc.SampleDesc.Quality = 0;
  texture_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  m_device->CreateCommittedResource(&create_heap_props_(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, &texture_desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&rv->texture));

  U64 upload_buffer_size;
  m_device->GetCopyableFootprints(&texture_desc, 0, 1, 0, NULL, NULL, NULL, &upload_buffer_size);

  D3D12_SUBRESOURCE_FOOTPRINT pitched_desc = {};
  int row_size = ci.width * convert_format_to_size_(ci.format);
  pitched_desc.Format = convert_format_to_dxgi_format(ci.format);
  pitched_desc.Width = ci.width;
  pitched_desc.Height = ci.height;
  pitched_desc.Depth = 1;
  pitched_desc.RowPitch = (row_size + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
  auto m_texture_subbuffer = allocate_sub_buffer_(&m_upload_buffer, pitched_desc.Height * pitched_desc.RowPitch, D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT);
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT placed_texture = {};
  placed_texture.Offset = (Sip)m_texture_subbuffer.cpu_p - (Sip)m_texture_subbuffer.buffer->cpu_p;
  placed_texture.Footprint = pitched_desc;
  for (int i = 0; i < pitched_desc.Height; ++i) {
    memcpy(m_texture_subbuffer.cpu_p + i * row_size, &ci.data[i * row_size], row_size);
  }
  D3D12_TEXTURE_COPY_LOCATION src = {};
  src.pResource = m_texture_subbuffer.buffer->buffer;
  src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  src.PlacedFootprint = placed_texture;
  D3D12_TEXTURE_COPY_LOCATION dest = {};
  dest.pResource = rv->texture;
  dest.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dest.SubresourceIndex = 0;
  m_cmd_allocators[m_frame_no]->Reset();
  m_cmd_list->Reset(m_cmd_allocators[m_frame_no], NULL);
  m_cmd_list->CopyTextureRegion(&dest, 0, 0, 0, &src, NULL);
  m_cmd_list->ResourceBarrier(1, &create_transition_barrier_(rv->texture, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE));
  m_cmd_list->Close();
  m_cmd_queue->ExecuteCommandLists(1, (ID3D12CommandList**)&m_cmd_list);
  wait_for_gpu_();
  return rv;
}

Resources_set_t* D3d12_t::create_resources_set(Allocator_t* allocator, const Resources_set_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_resources_set_t>();
  rv->uniform_buffer_count = ci.uniform_buffer_count;
  rv->sampler_count = ci.sampler_count;
  rv->image_count = ci.image_count;
  rv->visibility = ci.visibility;
  return rv;
}

Pipeline_layout_t* D3d12_t::create_pipeline_layout(Allocator_t* allocator, const Pipeline_layout_create_info_t& ci) {
  M_check_return_val(ci.set_count, NULL);
  auto rv = allocator->construct<D3d12_pipeline_layout_t>();
  Fixed_array_t<Fixed_array_t<D3D12_DESCRIPTOR_RANGE1, 4>, 8> ranges;
  ranges.resize(8);
  for (int i = 0; i < ci.set_count; ++i) {
    const auto* set = (D3d12_resources_set_t*)(ci.sets)[i];
    if (set->uniform_buffer_count) {
      D3D12_DESCRIPTOR_RANGE1 range = {};
      range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
      range.NumDescriptors = set->uniform_buffer_count;
      range.BaseShaderRegister = 0;
      range.RegisterSpace = i;
      range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
      range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      ranges[i].append(range);
    }
    if (set->sampler_count) {
      D3D12_DESCRIPTOR_RANGE1 range = {};
      range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
      range.NumDescriptors = set->sampler_count;
      range.BaseShaderRegister = 0;
      range.RegisterSpace = i;
      range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
      range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      ranges[i].append(range);
    }

    if (set->image_count) {
      D3D12_DESCRIPTOR_RANGE1 range = {};
      range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      range.NumDescriptors = set->image_count;
      range.BaseShaderRegister = 0;
      range.RegisterSpace = i;
      range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC;
      range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
      ranges[i].append(range);
    }
  }

  D3D12_ROOT_SIGNATURE_FLAGS root_sig_flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_AMPLIFICATION_SHADER_ROOT_ACCESS |
    D3D12_ROOT_SIGNATURE_FLAG_DENY_MESH_SHADER_ROOT_ACCESS;

  Fixed_array_t<D3D12_ROOT_PARAMETER1, 16> root_params;
  for (int i = 0; i < ci.set_count; ++i) {
    const auto* set = (D3d12_resources_set_t*)(ci.sets)[i];
    D3D12_ROOT_PARAMETER1 param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.DescriptorTable = {};
    param.DescriptorTable.NumDescriptorRanges = ranges[i].len();
    param.DescriptorTable.pDescriptorRanges = ranges[i].m_p;
    param.ShaderVisibility = convert_shader_stage_to_visibility(set->visibility);
    update_root_signature_flags(&root_sig_flags, set->visibility);
    root_params.append(param);
  }

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC desc = {};
  desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  desc.Desc_1_1.NumParameters = root_params.len();
  desc.Desc_1_1.pParameters = root_params.m_p;
  desc.Desc_1_1.NumStaticSamplers = 0;
  desc.Desc_1_1.pStaticSamplers = NULL;
  desc.Desc_1_1.Flags = root_sig_flags;
  ID3DBlob* signature;
  ID3DBlob* error;
  M_check_log_return_val(D3D12SerializeVersionedRootSignature(&desc, &signature, &error) == S_OK, NULL, "%s", error->GetBufferPointer());
  M_dx_check_return_val_(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rv->root_signature)), NULL);
  return rv;
}

Vertex_buffer_t* D3d12_t::create_vertex_buffer(Allocator_t* allocator, const Vertex_buffer_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_vertex_buffer_t>();
  rv->stride = ci.stride;
  rv->sub_buffer = allocate_sub_buffer_(&m_vertex_buffer, ci.size, ci.alignment);
  rv->p = rv->sub_buffer.cpu_p;
  return rv;
}

Render_target_t* D3d12_t::create_depth_stencil(Allocator_t* allocator, const Depth_stencil_create_info_t& ci) {
  D3D12_RESOURCE_DESC depth_tex_desc = {};
  depth_tex_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  depth_tex_desc.Alignment = 0;
  depth_tex_desc.Width = m_window->m_width;
  depth_tex_desc.Height = m_window->m_height;
  depth_tex_desc.DepthOrArraySize = 1;
  depth_tex_desc.MipLevels = 1;
  depth_tex_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depth_tex_desc.SampleDesc.Count = 1;
  depth_tex_desc.SampleDesc.Quality = 0;
  depth_tex_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  depth_tex_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

  D3D12_CLEAR_VALUE clear_value;
  clear_value.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  clear_value.DepthStencil.Depth = 1.0f;
  clear_value.DepthStencil.Stencil = 0;

  D3D12_HEAP_PROPERTIES heap_props = create_heap_props_(D3D12_HEAP_TYPE_DEFAULT);

  ID3D12Resource* depth_stencil;
  M_dx_check_return_val_(
      m_device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &depth_tex_desc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clear_value, IID_PPV_ARGS(&depth_stencil)),
      NULL);
  auto rv = allocator->construct<D3d12_render_target_t>();
  rv->state = e_resource_state_depth_write;
  rv->type = e_render_target_type_depth_stencil;
  rv->resource = depth_stencil;
  {
    rv->dsv_descriptor = allocate_descriptor_(&m_dsv_heap);
    D3D12_DEPTH_STENCIL_VIEW_DESC dsv_view_desc = {};
    dsv_view_desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    dsv_view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv_view_desc.Flags = D3D12_DSV_FLAG_NONE;
    dsv_view_desc.Texture2D.MipSlice = 0;
    m_device->CreateDepthStencilView(depth_stencil, &dsv_view_desc, rv->dsv_descriptor.cpu_handle);
  }
  return rv;
}

Render_pass_t* D3d12_t::create_render_pass(Allocator_t* allocator, const Render_pass_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_render_pass_t>();
  rv->rt_descs.init(allocator);
  rv->rt_descs.reserve(ci.render_target_count);
  rv->is_last = ci.is_last;
  rv->use_swapchain_render_target = ci.use_swapchain_render_target;
  rv->should_clear_render_target = ci.should_clear_render_target;
  for (int i = 0; i < ci.render_target_count; ++i) {
    const Render_target_description_t& rt_desc = ci.descs[i];
    rv->rt_descs.append(rt_desc);
  }
  if (ci.is_last) {
    rv->use_swapchain_render_target = true;
  }
  if (rv->use_swapchain_render_target) {
    rv->rt_descs.reserve(ci.render_target_count + 1);
    Render_target_description_t desc = {};
    desc.render_target = NULL;
    desc.render_pass_state = e_resource_state_render_target;
    if (ci.is_last) {
      desc.state_after = e_resource_state_present;
    } else {
      desc.state_after = e_resource_state_render_target;
    }
    rv->rt_descs.append(desc);
  }
  return rv;
}

Uniform_buffer_t* D3d12_t::create_uniform_buffer(Allocator_t* allocator, const Uniform_buffer_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_uniform_buffer_t>();
  rv->sub_buffer = allocate_sub_buffer_(&m_uniform_buffer, ci.size, ci.alignment);
  rv->descriptor = allocate_descriptor_(&m_cbv_srv_heap);
  D3D12_CONSTANT_BUFFER_VIEW_DESC desc = {};
  desc.BufferLocation = rv->sub_buffer.gpu_p;
  desc.SizeInBytes = rv->sub_buffer.size;
  m_device->CreateConstantBufferView(&desc, rv->descriptor.cpu_handle);
  rv->p = rv->sub_buffer.cpu_p;
  return rv;
}

Sampler_t* D3d12_t::create_sampler(Allocator_t* allocator, const Sampler_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_sampler_t>();
  rv->descriptor = allocate_descriptor_(&m_sampler_heap);
  D3D12_SAMPLER_DESC sampler_desc = {};
  sampler_desc.Filter = D3D12_FILTER_ANISOTROPIC;
  sampler_desc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  sampler_desc.MipLODBias = 0;
  sampler_desc.MaxAnisotropy = 0;
  sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
  sampler_desc.MinLOD = 0.0f;
  sampler_desc.MaxLOD = D3D12_FLOAT32_MAX;
  m_device->CreateSampler(&sampler_desc, rv->descriptor.cpu_handle);
  return rv;
}

Image_view_t* D3d12_t::create_image_view(Allocator_t* allocator, const Image_view_create_info_t& ci) {
  auto rv = allocator->construct<D3d12_image_view_t>();
  rv->descriptor = allocate_descriptor_(&m_cbv_srv_heap);
  ID3D12Resource* resource;
  if (ci.render_target) {
    auto d3d12_rt = (D3d12_render_target_t*)ci.render_target;
    resource = d3d12_rt->resource;
  } else if (ci.texture) {
    auto d3d12_texture = (D3d12_texture_t*)ci.texture;
    resource = d3d12_texture->texture;
  } else {
    M_logf_return_val(NULL, "One of |ci.render_target| or |ci.texture| has to have a valid value");
  }
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = convert_format_to_dxgi_format(ci.format);
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;
  srv_desc.Texture2D.PlaneSlice = 0;
  srv_desc.Texture2D.ResourceMinLODClamp = 0.f;
  m_device->CreateShaderResourceView(resource, &srv_desc, rv->descriptor.cpu_handle);
  return rv;
}

Shader_t* D3d12_t::compile_shader(Allocator_t* allocator, const Shader_create_info_t& ci) {
  ID3DBlob* blob;
  Path_t path_with_ext = ci.path;
  path_with_ext.m_path_str.append(M_txt(".cso"));
  M_dx_check_return_val_(D3DReadFileToBlob(path_with_ext.m_path, &blob), NULL);
  auto rv = allocator->construct<D3d12_shader_t>();
  rv->blob = blob;
  return rv;
}

Pipeline_state_object_t* D3d12_t::create_pipeline_state_object(Allocator_t* allocator, const Pipeline_state_object_create_info_t& ci) {
  Linear_allocator_t<> temp_allocator("dx12_temp_allocator");
  temp_allocator.init();
  M_scope_exit(temp_allocator.destroy());

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};
  pso_desc.pRootSignature = ((D3d12_pipeline_layout_t*)ci.pipeline_layout)->root_signature;

  if (ci.vs) {
    pso_desc.VS.pShaderBytecode = ((D3d12_shader_t*)ci.vs)->blob->GetBufferPointer();
    pso_desc.VS.BytecodeLength = ((D3d12_shader_t*)ci.vs)->blob->GetBufferSize();
  }
  if (ci.ps) {
    pso_desc.PS.pShaderBytecode = ((D3d12_shader_t*)ci.ps)->blob->GetBufferPointer();
    pso_desc.PS.BytecodeLength = ((D3d12_shader_t*)ci.ps)->blob->GetBufferSize();
  }

  pso_desc.BlendState.AlphaToCoverageEnable = FALSE;
  pso_desc.BlendState.IndependentBlendEnable = FALSE;
  for (int i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    D3D12_RENDER_TARGET_BLEND_DESC* rt_desc = &pso_desc.BlendState.RenderTarget[i];
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

  pso_desc.SampleMask = UINT_MAX;

  pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID,
  pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK,
  pso_desc.RasterizerState.FrontCounterClockwise = TRUE,
  pso_desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
  pso_desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
  pso_desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
  pso_desc.RasterizerState.DepthClipEnable = TRUE,
  pso_desc.RasterizerState.MultisampleEnable = FALSE,
  pso_desc.RasterizerState.AntialiasedLineEnable = FALSE,
  pso_desc.RasterizerState.ForcedSampleCount = 0,
  pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF,

  pso_desc.DepthStencilState = {};

  Dynamic_array_t<D3D12_INPUT_ELEMENT_DESC> elems;
  elems.init(&temp_allocator);
  elems.resize(ci.input_element_count);
  for (int i = 0; i < ci.input_element_count; ++i) {
    auto& elem = elems[i];
    auto& ci_elem = ci.input_elements[i];
    elem.SemanticName = ci_elem.semantic_name;
    elem.SemanticIndex = ci_elem.semantic_index;
    elem.Format = convert_format_to_dxgi_format(ci_elem.format);
    elem.InputSlot = ci_elem.input_slot;
    elem.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    elem.InstanceDataStepRate = 0;
  }
  pso_desc.InputLayout.NumElements = ci.input_element_count;
  pso_desc.InputLayout.pInputElementDescs = elems.m_p;
  pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pso_desc.NumRenderTargets = 1;
  pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pso_desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
  pso_desc.SampleDesc.Count = 1;
  if (ci.enable_depth) {
    pso_desc.DepthStencilState.DepthEnable = TRUE;
    pso_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    pso_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
  }
  pso_desc.DepthStencilState.StencilEnable = FALSE;
  ID3D12PipelineState* pso;
  M_dx_check_return_val_(m_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pso)), NULL);
  auto rv = allocator->construct<D3d12_pipeline_state_object_t>();
  rv->pso = pso;
  rv->root_signature = pso_desc.pRootSignature;
  return rv;
}

void D3d12_t::get_back_buffer() {
}

void D3d12_t::cmd_begin() {
  M_dx_check_return_(m_cmd_allocators[m_frame_no]->Reset());
  M_dx_check_return_(m_cmd_list->Reset(m_cmd_allocators[m_frame_no], NULL));
  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = 0;
  viewport.TopLeftY = 0;
  viewport.Width = (float)m_window->m_width;
  viewport.Height = (float)m_window->m_height;
  viewport.MinDepth = D3D12_MIN_DEPTH;
  viewport.MaxDepth = D3D12_MAX_DEPTH;

  D3D12_RECT scissor_rect = {};
  scissor_rect.left = 0;
  scissor_rect.top = 0;
  scissor_rect.right = m_window->m_width;
  scissor_rect.bottom = m_window->m_height;
  m_cmd_list->RSSetViewports(1, &viewport);
  m_cmd_list->RSSetScissorRects(1, &scissor_rect);
}

void D3d12_t::cmd_begin_render_pass(Render_pass_t* render_pass) {
  auto d3d12_render_pass = (D3d12_render_pass_t*)render_pass;
  if (d3d12_render_pass->use_swapchain_render_target) {
    d3d12_render_pass->rt_descs[d3d12_render_pass->rt_descs.len() - 1].render_target = &m_swapchain_rts[m_frame_no];
  }
  Fixed_array_t<D3D12_RESOURCE_BARRIER, 8> barriers;
  D3D12_CPU_DESCRIPTOR_HANDLE* depth_stencil_descriptor = NULL;
  Fixed_array_t<D3D12_CPU_DESCRIPTOR_HANDLE, 8> color_rt_descriptor_handles;
  for (auto& desc : d3d12_render_pass->rt_descs) {
    auto rt = (D3d12_render_target_t*)desc.render_target;
    ID3D12Resource* resource = rt->resource;
    D3D12_RESOURCE_STATES state_before = convert_resource_state_to_d3d12_resource_state(rt->state);
    D3D12_RESOURCE_STATES render_pass_state = convert_resource_state_to_d3d12_resource_state(desc.render_pass_state);
    if (state_before != render_pass_state) {
      barriers.append(create_transition_barrier_(resource, state_before, render_pass_state));
      rt->state = desc.render_pass_state;
    }
    if (rt->type == e_render_target_type_depth_stencil) {
      depth_stencil_descriptor = &rt->dsv_descriptor.cpu_handle;
    } else {
      color_rt_descriptor_handles.append(rt->rtv_descriptor.cpu_handle);
    }
  }
  if (barriers.len()) {
    m_cmd_list->ResourceBarrier(barriers.len(), barriers.m_p);
  }
  m_cmd_list->OMSetRenderTargets(color_rt_descriptor_handles.len(), color_rt_descriptor_handles.m_p, FALSE, depth_stencil_descriptor);
  if (d3d12_render_pass->should_clear_render_target) {
    for (const auto handle : color_rt_descriptor_handles) {
      float clear_color[] = {1.0f, 1.0f, 1.0f, 1.0f};
      m_cmd_list->ClearRenderTargetView(handle, clear_color, 0, NULL);
    }
    if (depth_stencil_descriptor) {
      m_cmd_list->ClearDepthStencilView(*depth_stencil_descriptor, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, NULL);
    }
  }
}

void D3d12_t::cmd_end_render_pass(Render_pass_t* render_pass) {
  auto d3d12_render_pass = (D3d12_render_pass_t*)render_pass;
  Fixed_array_t<D3D12_RESOURCE_BARRIER, 8> barriers;
  for (auto& desc : d3d12_render_pass->rt_descs) {
    auto rt = (D3d12_render_target_t*)desc.render_target;
    ID3D12Resource* resource = rt->resource;
    D3D12_RESOURCE_STATES render_pass_state = convert_resource_state_to_d3d12_resource_state(rt->state);
    D3D12_RESOURCE_STATES state_after = convert_resource_state_to_d3d12_resource_state(desc.state_after);
    if (render_pass_state != state_after) {
      barriers.append(create_transition_barrier_(resource, render_pass_state, state_after));
      rt->state = desc.state_after;
    }
  }
  if (barriers.len()) {
    m_cmd_list->ResourceBarrier(barriers.len(), barriers.m_p);
  }
}

void D3d12_t::cmd_set_pipeline_state(Pipeline_state_object_t* pso) {
  auto d3d12_pso = (D3d12_pipeline_state_object_t*)pso;
  m_cmd_list->SetPipelineState(d3d12_pso->pso);
  m_cmd_list->SetGraphicsRootSignature(d3d12_pso->root_signature);
  ID3D12DescriptorHeap* heaps[] = { m_cbv_srv_heap.heap, m_sampler_heap.heap };
  m_cmd_list->SetDescriptorHeaps(static_array_size(heaps), heaps);
}

void D3d12_t::cmd_set_vertex_buffer(Vertex_buffer_t* vb, int binding) {
  auto d3d12_vb = (D3d12_vertex_buffer_t*)vb;
  const D3d12_sub_buffer_t_& sub_buffer = d3d12_vb->sub_buffer;
  D3D12_VERTEX_BUFFER_VIEW vb_view = {};
  vb_view.BufferLocation = sub_buffer.gpu_p;
  vb_view.SizeInBytes =  sub_buffer.size;
  vb_view.StrideInBytes = vb->stride;
  m_cmd_list->IASetVertexBuffers(binding, 1, &vb_view);
}

void D3d12_t::cmd_set_uniform_buffer(Uniform_buffer_t* ub, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  auto d3d12_ub = (D3d12_uniform_buffer_t*)ub;
  m_cmd_list->SetGraphicsRootDescriptorTable(index, d3d12_ub->descriptor.gpu_handle);
}

void D3d12_t::cmd_set_sampler(Sampler_t* sampler, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  auto d3d12_sampler = (D3d12_sampler_t*)sampler;
  m_cmd_list->SetGraphicsRootDescriptorTable(index, d3d12_sampler->descriptor.gpu_handle);
}

void D3d12_t::cmd_set_image_view(Image_view_t* image_view, Pipeline_layout_t* pipeline_layout, Resources_set_t* set, int index) {
  auto d3d12_image_view = (D3d12_image_view_t*)image_view;
  m_cmd_list->SetGraphicsRootDescriptorTable(index, d3d12_image_view->descriptor.gpu_handle);
}

void D3d12_t::cmd_draw(int vertex_count, int first_vertex) {
  m_cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  m_cmd_list->DrawInstanced(vertex_count, 1, first_vertex, 0);
}

void D3d12_t::cmd_end() {
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

void D3d12_t::create_descriptor_heap_(D3d12_descriptor_heap_t_* dh, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags, U32 max_descriptor_count) {
  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type = type;
  desc.NumDescriptors = max_descriptor_count;
  desc.Flags = flags;
  desc.NodeMask = 0;
  M_dx_check_return_(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&dh->heap)));
  dh->increment_size = m_device->GetDescriptorHandleIncrementSize(type);
}

void D3d12_t::wait_for_gpu_() {
  // Wait for pending GPU work to complete.

  // Schedule a Signal command in the queue.
  M_dx_check_return_(m_cmd_queue->Signal(m_fence, m_fence_vals[m_frame_no]));

  // Wait until the fence has been processed.
  M_dx_check_return_(m_fence->SetEventOnCompletion(m_fence_vals[m_frame_no], m_fence_event));
  WaitForSingleObjectEx(m_fence_event, INFINITE, FALSE);

  // Increment the fence value for the current frame.
  ++m_fence_vals[m_frame_no];
}

#if M_compiler_is_clang()
#pragma clang diagnostic pop
#endif
