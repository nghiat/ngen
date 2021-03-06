//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/string.h"
#include "core/os.h"
#include "core/types.h"

#define M_max_path_len (256)

// TODO: sanitize "//"
template <typename T>
class Path_t_ {
public:
  Path_t_();
  Path_t_(const Cstring_t_<T>& path);
  Path_t_(const Path_t_<T>& other);
  Path_t_<T>& operator =(const Path_t_<T>& other);

  static Path_t_<T> from_char(const Cstring_t& path);
  Path_t_<char> get_path8() const;

  void replace_separator_();

  void update_path_str();

  bool is_dir() const;
  bool is_file() const;
  bool equals(const Path_t_<T>& other) const;

  Cstring_t_<T> get_name() const;
  Path_t_ get_parent_dir() const;
  Path_t_ get_absolute_path() const;
  Path_t_ join(const Cstring_t_<T>& subpath) const;

  static T s_native_separator_;
  T m_path[M_max_path_len] = {};
  Mstring_t_<T> m_path_str;
};

using Path_t = Path_t_<Os_char>;
using Path8_t = Path_t_<char>;
#if M_os_is_win()
using Path16_t = Path_t_<Os_char>;
#endif
