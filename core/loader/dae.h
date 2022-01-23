//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct Allocator;

class Dae_loader {
public:
  bool init(Allocator* allocator, const Os_char* path);
  void destroy();

  Dynamic_array<V4> m_vertices;
};
