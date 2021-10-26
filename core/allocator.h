//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

class Allocator {
public:
  Allocator(const char* name, Sz total_size) : m_name(name), m_total_size(total_size) {}
  virtual void al_destroy() = 0;
  virtual void* al_aligned_alloc(Sip size, Sip alignment) = 0;
  virtual void* al_realloc(void* p, Sip size) = 0;
  virtual void al_free(void* p) = 0;

  void* al_alloc(Sip size);
  void* al_alloc_zero(Sip size);

  const char* m_name = nullptr;
  /// Total size of the allocator in bytes.
  Sip m_total_size = 0;
  /// Allocations' sizes and supporting data of the allocator in bytes.
  Sip m_used_size = 0;
};
