//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/string.h"
#include "core/types.h"
#include "core/os.h"

#define M_max_path_len (256)

// TODO: sanitize "//"
class Path_t {
public:
  Path_t();
  Path_t(const Os_cstring_t& path);
#if M_os_is_win()
  Path_t(const Cstring_t& path);
  void replace_separator_();
#endif
  void update_path_str();

  bool is_dir() const;
  bool is_file() const;

  Path_t get_parent_dir() const;
  Path_t join(const Os_cstring_t& subpath) const;

  static Os_char s_native_separator_;
  Os_char m_path[M_max_path_len];
  Os_mstring_t m_path_str;
};
