//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

class Value {
public:
  Value(bool v);
  Value(int v);
  Value(float v);
  Value(char* v);

  bool v_get_bool() const;
  int v_get_int() const;
  float v_get_float() const;
  const char* v_get_string() const;

private:
  union {
    bool m_bool;
    int m_int;
    float m_float;
    char* m_string;
  };
  enum E_value_type {
    e_value_type_none = -1,

    e_value_type_bool,
    e_value_type_int,
    e_value_type_float,
    e_value_type_string,

    e_value_type_count
  };

  E_value_type m_value_type = e_value_type_none;
};
