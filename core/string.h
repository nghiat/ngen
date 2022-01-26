//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

template <typename T>
class String_t {
public:
  String_t();
  String_t(const T* str);
  String_t(const T* str, Sip len);
  bool ends_with(const T* str, Sip len);

  const T* m_str = NULL;
  Sip m_length = 0;
};
