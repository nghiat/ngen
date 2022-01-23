//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os_string.h"

struct Allocator;

struct Png_loader {
public:
  bool init(Allocator* allocator, const Os_char* path);
  void destroy();

  Allocator* m_allocator;
  U8* m_data = NULL;
  U32 m_width = 0;
  U32 m_height = 0;
  U32 m_bit_depth = 0;
  U32 m_bit_per_pixel = 0;
};
