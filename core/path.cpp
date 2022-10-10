//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path.h"

#include "core/log.h"
#include "core/string.h"

#if M_os_is_win()
#include <Windows.h>
#endif

template <>
char Path_t_<char>::s_native_separator_ = '/';

template <>
wchar_t Path_t_<wchar_t>::s_native_separator_ = L'\\';

template <typename T>
void replace_separator_(Mstring_t_<T>* str);

template <>
void replace_separator_<char>(Mstring_t_<char>* str) {
}

template <>
void replace_separator_<wchar_t>(Mstring_t_<wchar_t>* str) {
  str->replace(L'/', Path_t_<wchar_t>::s_native_separator_);
}

template <typename T>
Path_t_<T>::Path_t_() : m_path_str(m_path, M_max_path_len) {}

template <typename T>
Path_t_<T>::Path_t_(const Cstring_t_<T>& path) {
  m_path_str = Mstring_t_<T>(m_path, M_max_path_len);
  m_path_str.copy(path);
  replace_separator_(&m_path_str);
}

template <typename T>
Path_t_<T>::Path_t_(const Path_t_<T>& other) : Path_t_() {
  m_path_str.copy(other.m_path_str.to_const());
}

template <typename T>
Path_t_<T>& Path_t_<T>::operator=(const Path_t_<T>& other) {
  m_path_str.copy(other.m_path_str.to_const());
  return *this;
}

template <>
Path_t_<char> Path_t_<char>::from_char(const Cstring_t& path) {
  return Path_t_<char>(path);
}

#if M_os_is_win()
template <>
Path_t_<wchar_t> Path_t_<wchar_t>::from_char(const Cstring_t& path) {
  Path_t_<wchar_t> rv;
  int len = MultiByteToWideChar(CP_UTF8, 0, path.m_p, path.m_length, NULL, 0);
  M_check_return_val(len < M_max_path_len, rv);
  MultiByteToWideChar(CP_UTF8, 0, path.m_p, path.m_length, rv.m_path, M_max_path_len);
  rv.update_path_str();
  replace_separator_(&rv.m_path_str);
  return rv;
}
#endif // M_os_is_win()

template <>
Path_t_<char> Path_t_<char>::get_path8() const {
  return *this;
}

#if M_os_is_win()
template <>
Path_t_<char> Path_t_<wchar_t>::get_path8() const {
  Path_t_<char> rv;
  int len = WideCharToMultiByte(CP_UTF8, 0, m_path_str.m_p, m_path_str.m_length, NULL, 0, NULL, NULL);
  M_check_return_val(len < M_max_path_len, rv);
  WideCharToMultiByte(CP_UTF8, 0, m_path_str.m_p, m_path_str.m_length, rv.m_path, M_max_path_len, NULL, NULL);
  rv.update_path_str();
  return rv;
}
#endif // M_os_is_win()

template <typename T>
void Path_t_<T>::update_path_str() {
  m_path_str = Mstring_t_<T>(m_path);
  m_path_str.m_capacity = M_max_path_len;
}

template <typename T>
bool Path_t_<T>::is_dir() const {
  return m_path_str.ends_with(s_native_separator_);
}

template <typename T>
bool Path_t_<T>::is_file() const {
  return !is_dir();
}

template <typename T>
bool Path_t_<T>::equals(const Path_t_<T>& other) const {
  return m_path_str.equals(other.m_path_str.to_const());
}

template <typename T>
Cstring_t_<T> Path_t_<T>::get_name() const {
  Cstring_t_<T> rv = m_path_str.to_const();
  Sip index;
  if (rv.find_char_reverse(&index, s_native_separator_)) {
    rv = rv.get_substr(index);
  }
  return rv;
}

template <typename T>
Path_t_<T> Path_t_<T>::get_parent_dir() const {
  Path_t_<T> rv = *this;
  Mstring_t_<T>& rv_str = rv.m_path_str;
  if (rv_str.m_length) {
    // We ignore the last char just in case it is a separator
    rv_str.m_length--;
    Sip index;
    if (rv_str.find_char_reverse(&index, s_native_separator_)) {
      rv_str.m_p[index] = 0;
      rv_str.m_length = index;
    }
  }
  return rv;
}

template <typename T>
Path_t_<T> Path_t_<T>::join(const Cstring_t_<T>& subpath) const {
  Path_t_<T> rv = *this;
  rv.m_path_str.m_p = rv.m_path;
  if (is_file()) {
    rv.m_path_str.append(s_native_separator_);
  }
  rv.m_path_str.append(subpath);
  return rv;
}

template class Path_t_<char>;
#if M_os_is_win()
template class Path_t_<wchar_t>;
#endif
