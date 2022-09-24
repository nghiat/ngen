//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string.h"

#include "core/log.h"
#include "core/utils.h"

#include <string.h>
#include <wchar.h>

template <typename T>
static Sip get_str_len_(const T* str);
template <typename T>
static bool find_char_(Sip* o_index, const T* str, T c, Sip from, Sz len);

template <>
Sip get_str_len_<char>(const char* str) {
  return strlen(str);
}

template <>
Sip get_str_len_<wchar_t>(const wchar_t* str) {
  return wcslen(str);
}

template <>
bool find_char_<char>(Sip* o_index, const char* str, char c, Sip from, Sz len) {
  auto found = (const char*)memchr((const void*)(str + from), c, len);
  if (found) {
    maybe_assign(o_index, found - str);
    return true;
  }
  return false;
}

template <>
bool find_char_<wchar_t>(Sip* o_index, const wchar_t* str, wchar_t c, Sip from, Sz len) {
  auto found = wmemchr(str + from, c, len);
  if (found) {
    maybe_assign(o_index, found - str);
    return true;
  }
  return false;
}

template <typename T>
String_t_<T>::String_t_() {}

template <typename T>
String_t_<T>::String_t_(T& c) : m_p(&c), m_length(1), m_capacity(1) {}

template <typename T>
String_t_<T>::String_t_(T* str) : m_p(str) {
  if (str) {
    m_length = get_str_len_(str);
  }
  m_capacity = m_length;
}

template <typename T>
String_t_<T>::String_t_(T* str, Sip len) : m_p(str), m_length(len), m_capacity(len) {}

template <typename T>
String_t_<T>::String_t_(T* str, Sip len, Sip capacity) : m_p(str), m_length(len), m_capacity(capacity) {}

template <typename T>
bool String_t_<T>::ends_with(const String_t_<const T>& str) const {
  if (m_length == 0 || str.m_length == 0 || m_length < str.m_length) {
    return false;
  }
  return memcmp(m_p + m_length - str.m_length, str.m_p, str.m_length * sizeof(T)) == 0;
}

template <typename T>
bool String_t_<T>::operator==(const String_t_<const T>& str) const {
  return equals(str);
}

template <typename T>
bool String_t_<T>::equals(const String_t_<const T>& str) const {
  if (m_length != str.m_length) {
    return false;
  }
  return memcmp(m_p, str.m_p, m_length * sizeof(T)) == 0;
}

template <typename T>
bool String_t_<T>::find_substr(Sip* o_index, const String_t_<const T>& substr, Sip from) const {
  String_t_<T> rv;
  if (m_length == 0 || substr.m_length == 0 || m_length < substr.m_length) {
    return false;
  }
  Sip index = from;
  while (index + substr.m_length < m_length) {
    if (!find_char_(&index, m_p, substr.m_p[0], index, m_length - index)) {
      break;
    }
    if (substr.m_length == 1) {
      maybe_assign(o_index, index);
      return true;
    }
    if (index + substr.m_length > m_length) {
      break;
    }
    if (memcmp(m_p + index, substr.m_p, substr.m_length * sizeof(T)) == 0) {
      maybe_assign(o_index, index);
      return true;
    }
  }
  return false;
}

template <typename T>
bool String_t_<T>::find_char(Sip* o_index, T c, Sip from) const {
  return find_char_(o_index, m_p, c, from, m_length - from);
}

template <typename T>
bool String_t_<T>::find_char_reverse(Sip* o_index, T c, Sip from) const {
  if (from == -1) {
    from = m_length - 1;
  }
  for (Sip i = from; i >= 0; --i) {
    if (m_p[i] == c) {
      maybe_assign(o_index, i);
      return true;
    }
  }
  return false;
}

template <typename T>
String_t_<T> String_t_<T>::get_substr(Sip from, Sip to) const {
  String_t_<T> rv;
  rv.m_p = m_p + from;
  if (to == -1) {
    rv.m_length = m_length - from;
    rv.m_capacity = m_capacity - to;
  } else {
    rv.m_length = to - from;
    rv.m_capacity = to - from;
  }
  return rv;
}

template <typename T>
String_t_<const T> String_t_<T>::to_const() const {
  return String_t_<const T>(m_p, m_length, m_capacity);
}

template <typename T>
void String_t_<T>::replace(T c, T new_c, Sip from) {
  Sip index;
  while (this->find_char(&index, c, from)) {
    this->m_p[index] = new_c;
    from = index + 1;
  }
}

template <typename T>
void String_t_<T>::append(const String_t_<const T>& str) {
  M_check_return(m_length + str.m_length <= m_capacity);
  memcpy(m_p + m_length, str.m_p, str.m_length * sizeof(T));
  m_length += str.m_length;
  if (m_length < m_capacity) {
    m_p[this->m_length] = 0;
  }
}

template <typename T>
void String_t_<T>::copy(const String_t_<const T>& str) {
  M_check_return(str.m_length < m_capacity);
  memcpy(m_p, str.m_p, str.m_length * sizeof(T));
  m_length = str.m_length;
  m_p[this->m_length] = 0;
}

template <typename T>
Sz Hash_t<String_t_<T>>::operator()(const String_t_<T>& key) const {
  return fnv1((const U8*)key.m_p, key.m_length * sizeof(T));
}
