//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include <Windows.h>

extern Os_char g_exe_path_[M_max_path_len];
extern Os_char g_exe_dir_[M_max_path_len];

bool path_utils_init() {
  DWORD len = GetModuleFileName(NULL, g_exe_path_, M_max_path_len);
  for (DWORD i = len - 1; i >= 0; --i) {
    if (g_exe_path_[i] == L'\\') {
      memcpy(g_exe_dir_, g_exe_path_, (i + 2) * sizeof(wchar_t));
      g_exe_dir_[i + 1] = 0;
      break;
    }
  }
  return true;
}
