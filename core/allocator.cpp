//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/allocator.h"

#include <string.h>

void* Allocator::al_alloc(Sip size) {
  return al_aligned_alloc(size, 16);
}

void* Allocator::al_alloc_zero(Sip size) {
  void* p = al_aligned_alloc(size, 16);
  memset(p, 0, size);
  return p;
}
