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
  virtual void destroy() = 0;
  virtual void* aligned_alloc(Sip size, Sip alignment) = 0;
  virtual void* realloc(void* p, Sip size) = 0;
  virtual void free(void* p) = 0;

  void* alloc(Sip size);
  void* alloc_zero(Sip size);

  const char* m_name = nullptr;
  /// Total size of the allocator in bytes.
  Sip m_total_size = 0;
  /// The size of the allocator that has been exposed via alloc(), realloc(), free() (without the size used by internal data).
  Sip m_exposed_size = 0;
  /// The same with |m_exposed_size| but with the size of internal data.
  Sip m_used_size = 0;
};
