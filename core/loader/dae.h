//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/math/vec4.h"
#include "core/os_string.h"

struct Allocator;

class Dae_loader {
public:
  bool dae_init(Allocator* allocator, const Os_char* path);
  void dae_destroy();

  Dynamic_array<V4> m_vertices;
};
