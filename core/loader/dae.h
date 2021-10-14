//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct ngAllocator;

class DAELoader {
public:
  bool dae_init(ngAllocator* allocator, const OSChar* path);
  void dae_destroy();

private:
  DynamicArray<V4> m_vertices;
};
