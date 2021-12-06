//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"

#include "core/core_allocators.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/log.h"
#include "core/mono_time.h"
#include "core/path_utils.h"

#include <stdlib.h>

static void core_destroy_() {
  log_destroy();
  return;
}

bool core_init(const Os_char* log_path) {
  bool rv = true;
  rv &= mono_time_init();
  rv &= core_allocators_init();
  rv &= path_utils_init();
  rv &= File::init();
  Os_char abs_log_path[M_max_path_len];
  path_from_exe_dir(abs_log_path, log_path, M_max_path_len);
  rv &= log_init(abs_log_path);
  rv &= debug_init();

  atexit(core_destroy_);
  return rv;
}
