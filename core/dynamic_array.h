//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

class Allocator_t;

template <typename T>
class Dynamic_array_t {
public:
  bool init(Allocator_t* allocator);
  void destroy();
  Sip len() const;
  void reserve(Sip count);
  void resize(Sip count);
  void remove_range(Sip pos, Sip length);
  void remove_at(Sip pos);
  void insert_at(Sip index, const T& val);
  void append(const T& val);
  void append_array(const T* array, int len);
  T& operator[](Sz index);

  T* m_p = NULL;
  Allocator_t* m_allocator = NULL;
  Sip m_length = 0;
  Sip m_capacity = 0;
};
