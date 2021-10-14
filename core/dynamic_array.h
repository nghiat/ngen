//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

class ngAllocator;

template <typename T>
class DynamicArray {
public:
  bool da_init(ngAllocator* allocator);
  void da_destroy();
  SIP da_len() const;
  void da_reserve(SIP num);
  void da_resize(SIP num);
  void da_remove_range(SIP pos, SIP length);
  void da_remove_at(SIP pos);
  void da_insert_at(SIP index, const T& val);
  void da_append(const T& val);
  T& operator[](SZ index);
private:
  T* m_p = NULL;
  ngAllocator* m_allocator = NULL;
  SIP m_length = 0;
  SIP m_capacity = 0;
};

