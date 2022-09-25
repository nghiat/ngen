//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/gpu/gpu.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;

struct Dds_pixel_format_t {
  U32 size;
  U32 flags;
  U32 four_cc;
  U32 rgb_bit_count;
  U32 r_bit_mask;
  U32 g_bit_mask;
  U32 b_bit_mask;
  U32 a_bit_mask;
};

struct Dds_header_t {
  U32 size;
  U32 flags;
  U32 height;
  U32 width;
  U32 pitch_or_linear_size;
  U32 depth;
  U32 mip_map_count;
  U32 reserved1[11];
  Dds_pixel_format_t pixel_format;
  U32 caps;
  U32 caps2;
  U32 caps3;
  U32 caps4;
  U32 reserved2;
};

enum E_dxgi_format {
  e_dxgi_format_unknown = 0,
  e_dxgi_format_r32g32b32a32_typeless = 1,
  e_dxgi_format_r32g32b32a32_float = 2,
  e_dxgi_format_r32g32b32a32_uint = 3,
  e_dxgi_format_r32g32b32a32_sint = 4,
  e_dxgi_format_r32g32b32_typeless = 5,
  e_dxgi_format_r32g32b32_float = 6,
  e_dxgi_format_r32g32b32_uint = 7,
  e_dxgi_format_r32g32b32_sint = 8,
  e_dxgi_format_r16g16b16a16_typeless = 9,
  e_dxgi_format_r16g16b16a16_float = 10,
  e_dxgi_format_r16g16b16a16_unorm = 11,
  e_dxgi_format_r16g16b16a16_uint = 12,
  e_dxgi_format_r16g16b16a16_snorm = 13,
  e_dxgi_format_r16g16b16a16_sint = 14,
  e_dxgi_format_r32g32_typeless = 15,
  e_dxgi_format_r32g32_float = 16,
  e_dxgi_format_r32g32_uint = 17,
  e_dxgi_format_r32g32_sint = 18,
  e_dxgi_format_r32g8x24_typeless = 19,
  e_dxgi_format_d32_float_s8x24_uint = 20,
  e_dxgi_format_r32_float_x8x24_typeless = 21,
  e_dxgi_format_x32_typeless_g8x24_uint = 22,
  e_dxgi_format_r10g10b10a2_typeless = 23,
  e_dxgi_format_r10g10b10a2_unorm = 24,
  e_dxgi_format_r10g10b10a2_uint = 25,
  e_dxgi_format_r11g11b10_float = 26,
  e_dxgi_format_r8g8b8a8_typeless = 27,
  e_dxgi_format_r8g8b8a8_unorm = 28,
  e_dxgi_format_r8g8b8a8_unorm_srgb = 29,
  e_dxgi_format_r8g8b8a8_uint = 30,
  e_dxgi_format_r8g8b8a8_snorm = 31,
  e_dxgi_format_r8g8b8a8_sint = 32,
  e_dxgi_format_r16g16_typeless = 33,
  e_dxgi_format_r16g16_float = 34,
  e_dxgi_format_r16g16_unorm = 35,
  e_dxgi_format_r16g16_uint = 36,
  e_dxgi_format_r16g16_snorm = 37,
  e_dxgi_format_r16g16_sint = 38,
  e_dxgi_format_r32_typeless = 39,
  e_dxgi_format_d32_float = 40,
  e_dxgi_format_r32_float = 41,
  e_dxgi_format_r32_uint = 42,
  e_dxgi_format_r32_sint = 43,
  e_dxgi_format_r24g8_typeless = 44,
  e_dxgi_format_d24_unorm_s8_uint = 45,
  e_dxgi_format_r24_unorm_x8_typeless = 46,
  e_dxgi_format_x24_typeless_g8_uint = 47,
  e_dxgi_format_r8g8_typeless = 48,
  e_dxgi_format_r8g8_unorm = 49,
  e_dxgi_format_r8g8_uint = 50,
  e_dxgi_format_r8g8_snorm = 51,
  e_dxgi_format_r8g8_sint = 52,
  e_dxgi_format_r16_typeless = 53,
  e_dxgi_format_r16_float = 54,
  e_dxgi_format_d16_unorm = 55,
  e_dxgi_format_r16_unorm = 56,
  e_dxgi_format_r16_uint = 57,
  e_dxgi_format_r16_snorm = 58,
  e_dxgi_format_r16_sint = 59,
  e_dxgi_format_r8_typeless = 60,
  e_dxgi_format_r8_unorm = 61,
  e_dxgi_format_r8_uint = 62,
  e_dxgi_format_r8_snorm = 63,
  e_dxgi_format_r8_sint = 64,
  e_dxgi_format_a8_unorm = 65,
  e_dxgi_format_r1_unorm = 66,
  e_dxgi_format_r9g9b9e5_sharedexp = 67,
  e_dxgi_format_r8g8_b8g8_unorm = 68,
  e_dxgi_format_g8r8_g8b8_unorm = 69,
  e_dxgi_format_bc1_typeless = 70,
  e_dxgi_format_bc1_unorm = 71,
  e_dxgi_format_bc1_unorm_srgb = 72,
  e_dxgi_format_bc2_typeless = 73,
  e_dxgi_format_bc2_unorm = 74,
  e_dxgi_format_bc2_unorm_srgb = 75,
  e_dxgi_format_bc3_typeless = 76,
  e_dxgi_format_bc3_unorm = 77,
  e_dxgi_format_bc3_unorm_srgb = 78,
  e_dxgi_format_bc4_typeless = 79,
  e_dxgi_format_bc4_unorm = 80,
  e_dxgi_format_bc4_snorm = 81,
  e_dxgi_format_bc5_typeless = 82,
  e_dxgi_format_bc5_unorm = 83,
  e_dxgi_format_bc5_snorm = 84,
  e_dxgi_format_b5g6r5_unorm = 85,
  e_dxgi_format_b5g5r5a1_unorm = 86,
  e_dxgi_format_b8g8r8a8_unorm = 87,
  e_dxgi_format_b8g8r8x8_unorm = 88,
  e_dxgi_format_r10g10b10_xr_bias_a2_unorm = 89,
  e_dxgi_format_b8g8r8a8_typeless = 90,
  e_dxgi_format_b8g8r8a8_unorm_srgb = 91,
  e_dxgi_format_b8g8r8x8_typeless = 92,
  e_dxgi_format_b8g8r8x8_unorm_srgb = 93,
  e_dxgi_format_bc6h_typeless = 94,
  e_dxgi_format_bc6h_uf16 = 95,
  e_dxgi_format_bc6h_sf16 = 96,
  e_dxgi_format_bc7_typeless = 97,
  e_dxgi_format_bc7_unorm = 98,
  e_dxgi_format_bc7_unorm_srgb = 99,
  e_dxgi_format_ayuv = 100,
  e_dxgi_format_y410 = 101,
  e_dxgi_format_y416 = 102,
  e_dxgi_format_nv12 = 103,
  e_dxgi_format_p010 = 104,
  e_dxgi_format_p016 = 105,
  e_dxgi_format_420_opaque = 106,
  e_dxgi_format_yuy2 = 107,
  e_dxgi_format_y210 = 108,
  e_dxgi_format_y216 = 109,
  e_dxgi_format_nv11 = 110,
  e_dxgi_format_ai44 = 111,
  e_dxgi_format_ia44 = 112,
  e_dxgi_format_p8 = 113,
  e_dxgi_format_a8p8 = 114,
  e_dxgi_format_b4g4r4a4_unorm = 115,

  e_dxgi_format_p208 = 130,
  e_dxgi_format_v208 = 131,
  e_dxgi_format_v408 = 132,

  e_dxgi_format_sampler_feedback_min_mip_opaque = 189,
  e_dxgi_format_sampler_feedback_mip_region_used_opaqu= 190,

  e_dxgi_format_force_uint = 0xffffffff
};

enum E_d3d10_resource_dimension {
  e_d3d10_resource_dimension_unknown = 0,
  e_d3d10_resource_dimension_buffer = 1,
  e_d3d10_resource_dimension_texture1d = 2,
  e_d3d10_resource_dimension_texture2d = 3,
  e_d3d10_resource_dimension_texture3d = 4
};

struct Dds_header_dxt10_t {
  E_dxgi_format dxgi_format;
  E_d3d10_resource_dimension resource_dimension;
  U32 misc_flag;
  U32 array_size;
  U32 misc_flags2;
};

class Dds_loader_t {
public:
  Dds_loader_t(Allocator_t* allocator) : m_file_data(allocator) {}
  bool init(const Path_t& path);
  void destroy();

  Dynamic_array_t<U8> m_file_data;
  Dds_header_t* m_header = NULL;
  Dds_header_dxt10_t* m_header10 = NULL;
  U8* m_data = NULL;
  U8* m_data2 = NULL;
  E_format m_format;
};
