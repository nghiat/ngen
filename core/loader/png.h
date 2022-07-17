//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/gpu/gpu.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;

struct Png_loader_t {
public:
  bool init(Allocator_t* allocator, const Path_t& path);
  void destroy();

  Allocator_t* m_allocator;
  U8* m_data = NULL;
  U32 m_width = 0;
  U32 m_height = 0;
  U8 m_bit_depth = 0;
  U8 m_components_per_pixel = 0;
  U8 m_bytes_per_pixel = 0;
  E_format m_format;
};
