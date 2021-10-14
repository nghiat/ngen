//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec2.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct ngAllocator;

class OBJLoader {
public:
  bool obj_init(ngAllocator* allocator, const OSChar* path);
  void obj_destroy();

  DynamicArray<V4> m_vertices;
  DynamicArray<V2> m_uvs;
  DynamicArray<V4> m_normals;
};

