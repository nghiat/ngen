//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/allocator.h"
#include "core/log.h"
#include "core/os_string.h"

Os_char g_exe_path_[M_max_path_len];
Os_char g_exe_dir_[M_max_path_len];

Os_char* path_from_exe_dir(Os_char* out, const Os_char* sub_path, int len) {
  int exe_dir_len = os_str_get_len(g_exe_dir_);
  int sub_path_len = os_str_get_len(sub_path);
  M_check_return_val(exe_dir_len + sub_path_len < len, NULL);
  memcpy(out, g_exe_dir_, exe_dir_len * sizeof(Os_char));
  memcpy(out + exe_dir_len, sub_path, sub_path_len * sizeof(Os_char));
  out[exe_dir_len + sub_path_len] = 0;
  return out;
}
