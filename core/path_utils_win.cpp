//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include <Windows.h>

extern OSChar g_exe_path[MAX_PATH_LEN];
extern OSChar g_exe_dir[MAX_PATH_LEN];

bool path_utils_init() {
  DWORD len = GetModuleFileName(NULL, g_exe_path, MAX_PATH_LEN);
  for (DWORD i = len - 1; i >= 0; --i) {
    if (g_exe_path[i] == L'\\') {
      memcpy(g_exe_dir, g_exe_path, (i + 2) * sizeof(wchar_t));
      g_exe_dir[i + 1] = 0;
      break;
    }
  }
  return true;
}
