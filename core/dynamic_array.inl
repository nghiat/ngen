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
bool Dynamic_array<T>::init(Allocator* allocator) {
  m_allocator = allocator;
  return true;
}

template <typename T>
void Dynamic_array<T>::destroy() {
  if (m_p) {
    m_allocator->free(m_p);
  }
}

template <typename T>
Sip Dynamic_array<T>::len() const {
  return m_length;
}

template <typename T>
void Dynamic_array<T>::reserve(Sip num) {
  if (num <= m_capacity) {
    return;
  }
  if (!m_p) {
    m_p = (T*)m_allocator->alloc(num * sizeof(T));
  } else {
    m_p = (T*)m_allocator->realloc(m_p, num * sizeof(T));
  }
  M_check_log_return(m_p, "Can't reserve memory for Dynamic_array<T>");
  m_capacity = num;
}

template <typename T>
void Dynamic_array<T>::resize(Sip num) {
  reserve(num);
  m_length = num;
}

template <typename T>
void Dynamic_array<T>::remove_range(Sip pos, Sip length) {
  M_check_log_return(pos >= 0 && pos < m_length && pos + length < m_length, "Can't remove invalid rage");
  memmove(m_p + pos, m_p + pos + length, (m_length - pos - length) * sizeof(T));
  m_length -= length;
}

template <typename T>
void Dynamic_array<T>::remove_at(Sip pos) {
  remove_range(pos, 1);
}

template <typename T>
void Dynamic_array<T>::insert_at(Sip index, const T& val) {
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
void Dynamic_array<T>::append(const T& val) {
  insert_at(m_length, val);
}

template <typename T>
T& Dynamic_array<T>::operator[](Sz index) {
  return m_p[index];
}
