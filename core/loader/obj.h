//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec2.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct Allocator_t;

class Obj_loader_t {
public:
  bool init(Allocator_t* allocator, const Os_char* path);
  void destroy();

  Dynamic_array_t<V4_t> m_vertices;
  Dynamic_array_t<V2_t> m_uvs;
  Dynamic_array_t<V4_t> m_normals;
};

