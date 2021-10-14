//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

class ngAllocator {
public:
  ngAllocator(const char* name, SZ total_size) : m_name(name), m_total_size(total_size) {}
  virtual void al_destroy() = 0;
  virtual void* al_aligned_alloc(SIP size, SIP alignment) = 0;
  virtual void* al_realloc(void* p, SIP size) = 0;
  virtual void al_free(void* p) = 0;

  void* al_alloc(SIP size);
  void* al_alloc_zero(SIP size);

protected:
  const char* m_name = nullptr;
  /// Total size of the allocator in bytes.
  SIP m_total_size = 0;
  /// Allocations' sizes and supporting data of the allocator in bytes.
  SIP m_used_size = 0;
};
