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

#include <type_traits>

template <typename T, typename std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, char>, bool> = true>
Sip get_str_len_(T* str) {
  return strlen(str);
}

template <typename T, typename std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, wchar_t>, bool> = true>
Sip get_str_len_(T* str) {
  return wcslen(str);
}

template <typename T, typename std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, char>, bool> = true>
T* find_char_c_(T* str, T c, Sz len) {
  return (T*)memchr((const void*)str, c, len);
}

template <typename T, typename std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, wchar_t>, bool> = true>
T* find_char_c_(T* str, T c, Sz len) {
  return wmemchr(str, c, len);
}

template <typename T_char, typename T_string>
T_string String_crtp_t_<T_char, T_string>::find_substr(const String_t_<const T_char>& substr) const {
  return ((T_string*)this)->convert_string_t_substr_to_this_(((T_string*)this)->find_substr_(substr));
}

template <typename T_char, typename T_string>
T_string String_crtp_t_<T_char, T_string>::find_char(T_char c) const {
  return ((T_string*)this)->convert_string_t_substr_to_this_(((T_string*)this)->find_char_(c));
}

template <typename T_char, typename T_string>
T_string String_crtp_t_<T_char, T_string>::find_char_reverse(T_char c) const {
  return ((T_string*)this)->convert_string_t_substr_to_this_(((T_string*)this)->find_char_reverse_(c));
}

template <typename T>
String_t_<T>::String_t_() {}

template <typename T>
String_t_<T>::String_t_(T& c) : m_p(&c), m_length(1) {}

template <typename T>
String_t_<T>::String_t_(T* str) : m_p(str) {
  m_length = get_str_len_(str);
}

template <typename T>
String_t_<T>::String_t_(T* str, Sip len) : m_p(str), m_length(len) {}

template <typename T>
bool String_t_<T>::ends_with(const String_t_<const T>& str) const {
  if (m_length == 0 || str.m_length == 0 || m_length < str.m_length) {
    return false;
  }
  return memcmp(m_p + m_length - str.m_length, str.m_p, str.m_length * sizeof(T)) == 0;
}

template <typename T>
bool String_t_<T>::equals(const String_t_<const T>& str) const {
  if (m_length != str.m_length) {
    return false;
  }
  return memcmp(m_p, str.m_p, m_length * sizeof(T)) == 0;
}

template <typename T>
String_t_<T> String_t_<T>::find_substr_(const String_t_<const T>& substr) const {
  String_t_<T> rv;
  if (m_length == 0 || substr.m_length == 0 || m_length < substr.m_length) {
    return rv;
  }
  String_t_<T> temp = *this;
  while (temp.m_length >= substr.m_length) {
    temp = get_substr_from_offset_pointer_(find_char_c_<T>(temp.m_p, substr.m_p[0], temp.m_length));
    if (!temp.m_p) {
      break;
    }
    if (substr.m_length == 1) {
      rv = temp;
      break;
    }
    if (temp.m_length < substr.m_length) {
      break;
    }
    if (memcmp(temp.m_p, substr.m_p, substr.m_length * sizeof(T)) == 0) {
      rv = temp;
      break;
    }
  }
  return rv;
}

template <typename T>
String_t_<T> String_t_<T>::find_char_(T c) const {
  return get_substr_from_offset_pointer_(find_char_c_<T>(m_p, c, m_length));
}

template <typename T>
String_t_<T> String_t_<T>::find_char_reverse_(T c) const {
  T* p = NULL;
  for (int i = 0; i < m_length; ++i) {
    if (m_p[m_length - 1 - i] == c) {
      p = m_p + m_length - 1 - i;
      break;
    }
  }
  return get_substr_from_offset_pointer_(p);
}

template <typename T>
String_t_<T> String_t_<T>::get_substr_from_offset_pointer_(T* p) const {
  String_t_<T> rv;
  if (!p) {
    return rv;
  }
  Sip offset = p - this->m_p;
  M_check_return_val(offset >= 0 && offset < this->m_length, rv);
  rv.m_p = p;
  rv.m_length = this->m_length - offset;
  return rv;
}

template <typename T>
Const_string_t_<T> Const_string_t_<T>::convert_string_t_substr_to_this_(const String_t_<T>& str) const {
  Const_string_t_<T> rv;
  rv.m_p = str.m_p;
  rv.m_length = str.m_length;
  return rv;
}

template <typename T>
Mutable_string_t_<T>::Mutable_string_t_() {}

template <typename T>
Mutable_string_t_<T>::Mutable_string_t_(T* str) : String_t_<T>(str), m_capacity(this->m_length) {}

template <typename T>
Mutable_string_t_<T>::Mutable_string_t_(T* str, Sip cap) : String_t_<T>(str), m_capacity(cap) {}

template <typename T>
Mutable_string_t_<T>::operator String_t_<const T>() const {
  return String_t_<const T>(this->m_p, this->m_length);
}

template <typename T>
void Mutable_string_t_<T>::replace(T c, T new_c) {
  while (true) {
    Mutable_string_t_<T> substr = this->find_char(c);
    if (substr.m_p) {
      substr.m_p[0] = new_c;
    } else {
      break;
    }
  }
}

template <typename T>
void Mutable_string_t_<T>::append(const String_t_<const T>& str) {
  M_check_return(this->m_length + str.m_length <= m_capacity);
  memcpy(this->m_p + this->m_length, str.m_p, str.m_length * sizeof(T));
  this->m_length += str.m_length;
  append_null_if_possible();
}

template <typename T>
void Mutable_string_t_<T>::copy(const String_t_<const T>& str) {
  M_check_return(str.m_length < m_capacity);
  memcpy(this->m_p, str.m_p, str.m_length * sizeof(T));
  this->m_length = str.m_length;
  this->m_p[this->m_length] = 0;
}

template <typename T>
Mutable_string_t_<T> Mutable_string_t_<T>::convert_string_t_substr_to_this_(const String_t_<T>& str) const {
  Mutable_string_t_<T> rv;
  rv.m_p = str.m_p;
  rv.m_length = str.m_length;
  if (str.m_p) {
    rv.m_capacity = m_capacity - (str.m_p - this->m_p);
  }
  return rv;
}

template <typename T>
void Mutable_string_t_<T>::append_null_if_possible() {
  if (this->m_length < m_capacity) {
    this->m_p[this->m_length] = 0;
  }
}

template <typename T>
Mutable_string_t_<T>::Mutable_string_t_(const String_t_<T>& str) {
  *this = str;
}

template class String_t_<const char>;
template class String_t_<const wchar_t>;
template class String_t_<char>;
template class String_t_<wchar_t>;

template class Const_string_t_<const char>;
template class Const_string_t_<const wchar_t>;
template class Mutable_string_t_<char>;
template class Mutable_string_t_<wchar_t>;

template class String_crtp_t_<const char, Const_string_t_<const char>>;
template class String_crtp_t_<const wchar_t, Const_string_t_<const wchar_t>>;
template class String_crtp_t_<char, Mutable_string_t_<char>>;
template class String_crtp_t_<wchar_t, Mutable_string_t_<wchar_t>>;
