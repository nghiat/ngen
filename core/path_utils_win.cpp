//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/log.h"

#include <Windows.h>

Path_t g_exe_path;
Path_t g_exe_dir;

bool path_utils_init() {
  DWORD len = GetModuleFileName(NULL, g_exe_path.m_path, M_max_path_len);
  M_check_return_val(len < M_max_path_len, false);
  g_exe_path.update_path_str();
  g_exe_dir = g_exe_path.get_parent_dir();
  return true;
}
