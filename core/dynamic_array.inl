//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"

#include "core/allocator.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

template <typename T>
bool Dynamic_array_t<T>::init(Allocator_t* allocator) {
  m_allocator = allocator;
  return true;
}

template <typename T>
void Dynamic_array_t<T>::destroy() {
  if (m_p) {
    m_allocator->free(m_p);
  }
}

template <typename T>
Sip Dynamic_array_t<T>::len() const {
  return m_length;
}

template <typename T>
void Dynamic_array_t<T>::reserve(Sip count) {
  if (count <= m_capacity) {
    return;
  }
  if (!m_p) {
    m_p = (T*)m_allocator->alloc(count * sizeof(T));
  } else {
    m_p = (T*)m_allocator->realloc(m_p, count * sizeof(T));
  }
  M_check_log_return(m_p, "Can't reserve memory for Dynamic_array_t<T>");
  m_capacity = count;
}

template <typename T>
void Dynamic_array_t<T>::resize(Sip count) {
  reserve(count);
  m_length = count;
}

template <typename T>
void Dynamic_array_t<T>::remove_range(Sip pos, Sip length) {
  M_check_log_return(pos >= 0 && pos < m_length && pos + length < m_length, "Can't remove invalid rage");
  memmove(m_p + pos, m_p + pos + length, (m_length - pos - length) * sizeof(T));
  m_length -= length;
}

template <typename T>
void Dynamic_array_t<T>::remove_at(Sip pos) {
  remove_range(pos, 1);
}

template <typename T>
void Dynamic_array_t<T>::insert_at(Sip index, const T& val) {
  if (m_length == m_capacity) {
    // TODO lower the factor when m_capacity is big
    reserve((m_capacity + 1) * 3 / 2);
  }
  if (index < m_length) {
    memmove(m_p + index + 1, m_p + index, (m_length - index) * sizeof(T));
  }
  m_p[index] = val;
  m_length += 1;
}

template <typename T>
void Dynamic_array_t<T>::append(const T& val) {
  insert_at(m_length, val);
}

template <typename T>
void Dynamic_array_t<T>::append_array(const T* array, int len) {
  Sip old_len = m_length;
  resize(m_length + len);
  memcpy(m_p + old_len, array, len * sizeof(T));
}

template <typename T>
T& Dynamic_array_t<T>::operator[](Sz index) {
  return m_p[index];
}
