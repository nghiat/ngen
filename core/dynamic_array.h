//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

class Allocator;

template <typename T>
class Dynamic_array {
public:
  bool init(Allocator* allocator);
  void destroy();
  Sip len() const;
  void reserve(Sip count);
  void resize(Sip count);
  void remove_range(Sip pos, Sip length);
  void remove_at(Sip pos);
  void insert_at(Sip index, const T& val);
  void append(const T& val);
  T& operator[](Sz index);

  T* m_p = NULL;
  Allocator* m_allocator = NULL;
  Sip m_length = 0;
  Sip m_capacity = 0;
};
