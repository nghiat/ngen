//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path.h"

#include "core/log.h"

#include <wchar.h>

#include <Windows.h>

#if M_os_is_win()
Os_char Path_t::s_native_separator_ = L'\\';
#elif
Os_char Path_t::s_native_separator_ = '/';
#else
#error "?"
#endif

Path_t::Path_t() : m_path_str(m_path, M_max_path_len) {}

#if M_os_is_win()
Path_t::Path_t(const Os_cstring_t& path) {
  m_path_str = Os_mstring_t(m_path);
  m_path_str.copy(path);
  replace_separator_();
}

Path_t::Path_t(const Cstring_t& path) {
  int len = MultiByteToWideChar(CP_UTF8, 0, path.m_p, path.m_length, NULL, 0);
  M_check_return(len < M_max_path_len);
  MultiByteToWideChar(CP_UTF8, 0, path.m_p, path.m_length, m_path, M_max_path_len);
  m_path_str = Os_mstring_t(m_path);
  replace_separator_();
}

void Path_t::replace_separator_() {
  m_path_str.replace(L'/', s_native_separator_);
}
#endif

void Path_t::update_path_str() {
  m_path_str = Os_mstring_t(m_path, M_max_path_len);
}

bool Path_t::is_dir() const {
  return m_path_str.ends_with(s_native_separator_);
}

bool Path_t::is_file() const {
  return !is_dir();
}

Path_t Path_t::get_parent_dir() const {
  Path_t rv = *this;
  Os_mstring_t& rv_str = rv.m_path_str;
  if (rv_str.m_length) {
    // We ignore the last char just in case it is a separator
    rv_str.m_length--;
    Os_mstring_t temp = rv_str.find_char_reverse(s_native_separator_);
    if (temp.m_p) {
      temp.m_p[0] = 0;
      rv_str.m_length -= temp.m_length;
    }
  }
  return rv;
}

Path_t Path_t::join(const Os_cstring_t& subpath) const {
  Path_t rv = *this;
  rv.m_path_str.m_p = rv.m_path;
  rv.m_path_str.append(s_native_separator_);
  rv.m_path_str.append(subpath);
  return rv;
}
