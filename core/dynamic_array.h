//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

class Allocator;

template <typename T>
class Dynamic_array {
public:
  bool da_init(Allocator* allocator);
  void da_destroy();
  Sip da_len() const;
  void da_reserve(Sip num);
  void da_resize(Sip num);
  void da_remove_range(Sip pos, Sip length);
  void da_remove_at(Sip pos);
  void da_insert_at(Sip index, const T& val);
  void da_append(const T& val);
  T& operator[](Sz index);

  T* m_p = NULL;
  Allocator* m_allocator = NULL;
  Sip m_length = 0;
  Sip m_capacity = 0;
};

