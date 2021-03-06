//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path.h"

#include <Windows.h>

template <typename T>
Path_t_<T> Path_t_<T>::get_absolute_path() const {
  Path_t_<T> rv;
  GetFullPathName(m_path, M_max_path_len, rv.m_path, NULL);
  rv.update_path_str();
  return rv;
}

template class Path_t_<wchar_t>;
