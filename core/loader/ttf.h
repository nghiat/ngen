//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;

class Ttf_loader_t {
public:
  Ttf_loader_t(Allocator_t* allocator);
  bool init(const Path_t& path);
  void destroy();
};
