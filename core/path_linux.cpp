//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path.h"

#include <stdlib.h>

template <>
Path_t_<char> Path_t_<char>::get_absolute_path() const {
  Path_t_<char> rv;
  realpath(m_path, rv.m_path);
  rv.update_path_str();
  return rv;
}
