//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"

#include "core/command_line.h"
#include "core/core_allocators.h"
#include "core/debug.h"
#include "core/file.h"
#include "core/log.h"
#include "core/mono_time.h"
#include "core/path_utils.h"

bool core_init(const Os_char* log_path) {
  bool rv = true;
  rv &= mono_time_init();
  rv &= core_allocators_init();
  Command_line_t::init();
  rv &= path_utils_init();
  rv &= File_t::init();
  Path_t final_log_path = g_exe_dir.join(log_path);
  rv &= log_init(final_log_path.m_path);
  rv &= debug_init();
  return rv;
}

void core_destroy() {
  log_destroy();
  core_allocators_destroy();
  return;
}
