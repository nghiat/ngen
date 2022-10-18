//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/log.h"

#include <stdlib.h>
#include <string.h>

template <typename T, Sz T_capacity>
Sip Fixed_array_t<T, T_capacity>::len() const {
  return m_length;
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::resize(Sip count) {
  M_check_return(count <= T_capacity);
  m_length = count;
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::insert_at(Sip index, const T& val) {
  M_check_return(m_length < T_capacity);
  if (index < m_length) {
    memmove(m_p + index + 1, m_p + index, (m_length - index) * sizeof(T));
  }
  m_p[index] = val;
  m_length += 1;
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::append(const T& val) {
  insert_at(m_length, val);
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::append_unique(const T& val) {
  for (int i = 0; i < m_length; ++i) {
    if (val == m_p[i]) {
      return;
    }
  }
  append(val);
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::append_array(const T* array, int len) {
  Sip old_len = m_length;
  resize(m_length + len);
  memcpy(m_p + old_len, array, len * sizeof(T));
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::remove_range(Sip pos, Sip length) {
  M_check_log_return(pos >= 0 && pos < m_length && pos + length <= m_length, "Can't remove invalid rage");
  memmove(m_p + pos, m_p + pos + length, (m_length - pos - length) * sizeof(T));
  m_length -= length;
}

template <typename T, Sz T_capacity>
void Fixed_array_t<T, T_capacity>::remove_at(Sip pos) {
  remove_range(pos, 1);
}

template <typename T, Sz T_capacity>
T& Fixed_array_t<T, T_capacity>::operator[](Sz index) {
  M_check(index < m_length);
  return m_p[index];
}

template <typename T, Sz T_capacity>
const T& Fixed_array_t<T, T_capacity>::operator[](Sz index) const {
  M_check(index < m_length);
  return m_p[index];
}

template <typename T, Sz T_capacity>
T* Fixed_array_t<T, T_capacity>::begin() {
  return &m_p[0];
}

template <typename T, Sz T_capacity>
T* Fixed_array_t<T, T_capacity>::end() {
  return &m_p[0] + m_length;
}
