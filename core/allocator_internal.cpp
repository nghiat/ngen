//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/allocator_internal.h"

#include "core/log.h"

AllocHeader* get_allocation_header(void* p) {
  return (AllocHeader*)p - 1;
}

U8* align_forward(U8* p, SIP alignment) {
  if (alignment == 1)
    return p;
  return (U8*)(((size_t)p + alignment - 1) & ~(size_t)(alignment - 1));
}

bool check_aligned_alloc(SIP size, SIP alignment) {
  CHECK_RETURN_VAL(size && alignment, false);

  // alignment has to be power of two.
  if (alignment & (alignment - 1)) {
    return false;
  }
  return true;
}

bool check_p_in_dev(void* p) {
  CHECK_RETURN_VAL(p, false);

#if IS_DEV()
  AllocHeader* header = get_allocation_header(p);
  if (header->p != p) {
    return false;
  }
#endif
  return true;
}
