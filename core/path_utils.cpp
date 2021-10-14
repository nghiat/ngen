//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/allocator.h"
#include "core/log.h"
#include "core/os_string.h"

OSChar g_exe_path[MAX_PATH_LEN];
OSChar g_exe_dir[MAX_PATH_LEN];

OSChar* path_from_exe_dir(OSChar* out, const OSChar* sub_path, int len) {
  int exe_dir_len = os_str_get_len(g_exe_dir);
  int sub_path_len = os_str_get_len(sub_path);
  CHECK_RETURN_VAL(exe_dir_len + sub_path_len < len, NULL);
  memcpy(out, g_exe_dir, exe_dir_len * sizeof(OSChar));
  memcpy(out + exe_dir_len, sub_path, sub_path_len * sizeof(OSChar));
  out[exe_dir_len + sub_path_len] = 0;
  return out;
}
