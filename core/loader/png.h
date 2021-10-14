//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os_string.h"

struct ngAllocator;

struct PNGLoader {
public:
  bool png_init(ngAllocator* allocator, const OSChar* path);
  void png_destroy();

private:
  ngAllocator* m_allocator;
  U8* m_data = NULL;
  U32 m_width = 0;
  U32 m_height = 0;
  U32 m_bit_depth = 0;
  U32 m_bit_per_pixel = 0;
};
