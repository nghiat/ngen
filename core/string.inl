//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/string.h"

#include <string.h>
#include <wchar.h>

template <typename T>
String_t<T>::String_t() {}

template <>
String_t<char>::String_t(const char* str) : m_str(str) {
  m_length = strlen(str);
}

template <>
String_t<wchar_t>::String_t(const wchar_t* str) : m_str(str) {
  m_length = wcslen(str);
}

template <typename T>
String_t<T>::String_t(const T* str, Sip len) : m_str(str), m_length(len) {
}

template <typename T>
bool String_t<T>::ends_with(const T* str, Sip len) {
  if (len > m_length || len == 0) {
    return false;
  }
  return memcmp(m_str + m_length - len, str, len * sizeof(T)) == 0;
}
