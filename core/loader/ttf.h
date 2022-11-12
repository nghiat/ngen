//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec2.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;

struct Glyph_t {
  U8* texture;
  U16 offset_x;
  U16 offset_y;
};

class Ttf_loader_t {
public:
  Ttf_loader_t(Allocator_t* allocator);
  bool init(const Path_t& path);
  void get_glyph(Glyph_t* glyph, const char c, int height_in_pixel);
  void destroy();

  Allocator_t* m_allocator;
  V2_t m_bottom_left;
  V2_t m_top_right;

  const U8* m_head_table;
  const U8* m_cmap_table;
  const U8* m_loca_table;
  const U8* m_glyf_table;
  const U8* m_hhea_table;
  const U8* m_hmtx_table;
};
