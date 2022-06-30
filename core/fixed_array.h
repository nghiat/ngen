//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

template <typename T, Sz T_capacity>
class Fixed_array_t {
public:
  using T_value = T;
  Sip len() const;
  void resize(Sip count);
  void insert_at(Sip index, const T& val);
  void append(const T& val);
  void append_array(const T* array, int len);
  void remove_range(Sip pos, Sip length);
  void remove_at(Sip pos);
  T& operator[](Sz index);
  const T& operator[](Sz index) const;

// iterator (for each)
  T* begin();
  T* end();

  T m_p[T_capacity];
  Sip m_length = 0;
};
