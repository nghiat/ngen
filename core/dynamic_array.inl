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
bool DynamicArray<T>::da_init(ngAllocator* allocator) {
  m_allocator = allocator;
  return true;
}

template <typename T>
void DynamicArray<T>::da_destroy() {
  if (m_p) {
    m_allocator->al_free(m_p);
  }
}

template <typename T>
SIP DynamicArray<T>::da_len() const {
  return m_length;
}

template <typename T>
void DynamicArray<T>::da_reserve(SIP num) {
  if (num <= m_capacity) {
    return;
  }
  if (!m_p) {
    m_p = (T*)m_allocator->al_alloc(num * sizeof(T));
  } else {
    m_p = (T*)m_allocator->al_realloc(m_p, num * sizeof(T));
  }
  CHECK_LOG_RETURN(m_p, "Can't reserve memory for DynamicArray<T>");
  m_capacity = num;
}

template <typename T>
void DynamicArray<T>::da_resize(SIP num) {
  da_reserve(num);
  m_length = num;
}

template <typename T>
void DynamicArray<T>::da_remove_range(SIP pos, SIP length) {
  CHECK_LOG_RETURN(pos >= 0 && pos < m_length && pos + length < m_length, "Can't remove invalid rage");
  memmove(m_p + pos, m_p + pos + length, (m_length - pos - length) * sizeof(T));
  m_length -= length;
}

template <typename T>
void DynamicArray<T>::da_remove_at(SIP pos) {
  da_remove_range(pos, 1);
}

template <typename T>
void DynamicArray<T>::da_insert_at(SIP index, const T& val) {
  if (m_length == m_capacity) {
    da_reserve((m_capacity + 1) * 3 / 2);
  }
  if (index < m_length) {
    memmove(m_p + index + 1, m_p + index, (m_length - index) * sizeof(T));
  }
  m_p[index] = val;
  m_length += 1;
}

template <typename T>
void DynamicArray<T>::da_append(const T& val) {
  da_insert_at(m_length, val);
}

template <typename T>
T& DynamicArray<T>::operator[](SZ index) {
  return m_p[index];
}
