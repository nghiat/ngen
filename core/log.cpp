//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/log.h"

#include "core/file.h"
#include "core/linear_allocator.h"
#include "core/utils.h"

#include <stdarg.h>
#include <stdlib.h>

#if M_os_is_win()
#include <Windows.h>
#endif

static const char* gc_log_level_strings_[] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "FATAL",
    "TEST",
};

bool g_is_log_in_testing = false;
static File_t g_log_file_;
static bool g_log_inited_ = false;

void ng_log_(E_log_level_ level, const char* file, int line, const char* format, ...) {
  if (!g_log_inited_) {
    return;
  }
  Linear_allocator_t<> temp_allocator("log_temp_allocator");
  M_scope_exit(temp_allocator.destroy());

  // FILE(LINE) for visual studio click to go to location.
  int log_len = 0;
  const char* log_prefix_format = "%s(%d): %s: ";
  const char* level_str = gc_log_level_strings_[(int)level];
  int prefix_len = snprintf(NULL, 0, log_prefix_format, file, line, level_str);
  log_len += prefix_len;
  char* log_buffer = (char*)temp_allocator.alloc(log_len + 1);
  snprintf(log_buffer, log_len + 1, log_prefix_format, file, line, level_str);

  va_list argptr, argptr2;
  va_start(argptr, format);
  va_copy(argptr2, argptr);
  // +1 for new line char.
  int msg_len = vsnprintf(NULL, 0, format, argptr) + 1;
  va_end(argptr);
  log_buffer = (char*)temp_allocator.realloc(log_buffer, log_len + msg_len + 1);
  va_start(argptr2, format);
  vsnprintf(log_buffer + log_len, msg_len + 1, format, argptr2);
  va_end(argptr2);
  log_len += msg_len;
  log_buffer[log_len - 1] = '\n';
  log_buffer[log_len] = 0;

  if ((level == e_log_level_fatal || level == e_log_level_test) && !debug_is_debugger_attached()) {
    const char* trace_format = "StackTraces:\n%s\n";
    char trace[M_max_stack_trace_length_];
    debug_get_stack_trace(trace, M_max_stack_trace_length_);
    int trace_len = snprintf(NULL, 0, trace_format, trace);
    log_buffer = (char*)temp_allocator.realloc(log_buffer, log_len + trace_len + 1);
    snprintf(log_buffer + log_len, trace_len + 1, trace_format, trace);
    log_len += trace_len;
  }
  // Log to stream
  FILE* stream;
  if (level == e_log_level_info || level == e_log_level_debug) {
    stream = stdout;
  } else {
    stream = stderr;
  }
  if (level == e_log_level_test || !g_is_log_in_testing) {
    fprintf(stream, "%s", log_buffer);
  }
#if M_os_is_win()
  OutputDebugStringA(log_buffer);
#endif
  g_log_file_.write(NULL, log_buffer, log_len);
}

bool log_init(const Os_char* log_path) {
  g_log_file_.init();
  g_log_file_.open(log_path, e_file_mode_append);
  g_log_inited_ = g_log_file_.is_valid();
  return g_log_inited_;
}

void log_destroy() {
  if (g_log_inited_) {
    g_log_file_.close();
  }
  g_log_inited_ = false;
}
