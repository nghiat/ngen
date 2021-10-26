//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/allocator_internal.h"

#include "core/log.h"

Alloc_header_* get_allocation_header_(void* p) {
  return (Alloc_header_*)p - 1;
}

U8* align_forward_(U8* p, Sip alignment) {
  if (alignment == 1)
    return p;
  return (U8*)(((size_t)p + alignment - 1) & ~(size_t)(alignment - 1));
}

bool check_aligned_alloc_(Sip size, Sip alignment) {
  M_check_return_val(size && alignment, false);

  // alignment has to be power of two.
  if (alignment & (alignment - 1)) {
    return false;
  }
  return true;
}

bool check_p_in_dev_(void* p) {
  M_check_return_val(p, false);

#if M_is_dev()
  Alloc_header_* header = get_allocation_header_(p);
  if (header->p != p) {
    return false;
  }
#endif
  return true;
}
