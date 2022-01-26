//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/value.h"

#include "core/log.h"

Value_t::Value_t() {}

Value_t::Value_t(bool v) : m_bool(v), m_value_type(e_value_type_bool) {}

Value_t::Value_t(int v) : m_int(v), m_value_type(e_value_type_int) {}

Value_t::Value_t(float v) : m_float(v), m_value_type(e_value_type_float) {}

Value_t::Value_t(char* v) : m_string(v), m_value_type(e_value_type_string) {}

bool Value_t::is_valid() const {
  return m_value_type == e_value_type_none;
}

bool Value_t::get_bool() const {
  M_check(m_value_type == e_value_type_bool);
  return m_bool;
}

int Value_t::get_int() const {
  M_check(m_value_type == e_value_type_int);
  return m_int;
}

float Value_t::get_float() const {
  M_check(m_value_type == e_value_type_float);
  return m_float;
}

const char* Value_t::get_string() const {
  M_check(m_value_type == e_value_type_string);
  return m_string;
}
