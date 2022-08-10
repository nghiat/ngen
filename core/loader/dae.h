//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec4.h"
#include "core/types.h"

struct Allocator_t;

class Dae_loader_t {
public:
  Dae_loader_t(Allocator_t* allocator);
  bool init(const Os_char* path);
  void destroy();

  Dynamic_array_t<V4_t> m_vertices;
};
