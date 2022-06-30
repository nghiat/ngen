//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string.h"

#pragma once

enum E_value_type {
  e_value_type_none = -1,

  e_value_type_bool,
  e_value_type_int,
  e_value_type_float,
  e_value_type_string,

  e_value_type_count
};

class Value_t {
public:
  Value_t();
  Value_t(bool v);
  Value_t(int v);
  Value_t(float v);
  Value_t(char* v);

  bool is_valid() const;
  bool get_bool() const;
  int get_int() const;
  float get_float() const;
  Cstring_t get_string() const;

// private:
  union {
    bool m_bool;
    int m_int;
    float m_float;
    char* m_string;
    const char* m_const_string = NULL;
  };
  E_value_type m_value_type = e_value_type_none;
};
