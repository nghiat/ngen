//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/path_utils.h"

#include "core/allocator.h"

#include <string.h>

#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

extern Os_char g_exe_path_[M_max_path_len];
extern Os_char g_exe_dir_[M_max_path_len];

bool path_utils_init() {
  char exe_path[M_max_path_len];
  ssize_t len = readlink("/proc/self/exe", g_exe_path_, M_max_path_len);
  for (ssize_t i = len - 1; i >= 0; --i) {
    if (g_exe_path_[i] == '/') {
      memcpy(g_exe_dir_, g_exe_path_, i + 2);
      g_exe_dir_[i + 1] = 0;
      break;
    }
  }
  return true;
}
