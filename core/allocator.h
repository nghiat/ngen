//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"
#include <new>

class Allocator_t {
public:
  Allocator_t(const char* name, Sz total_size) : m_name(name), m_total_size(total_size) {}
  virtual void destroy() = 0;
  virtual void* aligned_alloc(Sip size, Sip alignment) = 0;
  virtual void* realloc(void* p, Sip size) = 0;
  virtual void free(void* p) = 0;
  template <typename T_class, typename... T_args>
  T_class* construct(T_args... args);

  void* alloc(Sip size);
  template <typename T>
  T* alloc();
  void* alloc_zero(Sip size);

  const char* m_name = nullptr;
  /// Total size of the allocator in bytes.
  Sip m_total_size = 0;
  /// Size of used memory in the allocator including internal data.
  Sip m_used_size = 0;
};

template <typename T_class, typename... T_args>
T_class* Allocator_t::construct(T_args... args) {
  void* p = alloc(sizeof(T_class));
  return new (p) T_class(args...);
}

template <typename T>
T* Allocator_t::alloc() {
  return (T*)alloc(sizeof(T));
}
