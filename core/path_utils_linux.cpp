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

extern OSChar g_exe_path[MAX_PATH_LEN];
extern OSChar g_exe_dir[MAX_PATH_LEN];

bool path_utils_init() {
  char exe_path[MAX_PATH_LEN];
  ssize_t len = readlink("/proc/self/exe", g_exe_path, MAX_PATH_LEN);
  for (ssize_t i = len - 1; i >= 0; --i) {
    if (g_exe_path[i] == '/') {
      memcpy(g_exe_dir, g_exe_path, i + 2);
      g_exe_dir[i + 1] = 0;
      break;
    }
  }
  return true;
}
