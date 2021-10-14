//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/log.h"

#include "core/file.h"
#include "core/linear_allocator.inl"
#include "core/utils.h"

#include <stdarg.h>
#include <stdlib.h>

#if OS_WIN()
#include <Windows.h>
#endif

static const char* gc_log_level_strings[] = {
    "INFO",
    "DEBUG",
    "WARNING",
    "FATAL",
};

static ngFile g_log_file;
static bool g_log_inited = false;

void log_internal(ELogLevel level, const char* file, int line, const char* format, ...) {
  if (!g_log_inited) {
    return;
  }
  LinearAllocator<> temp_allocator("log_temp_allocator");
  temp_allocator.la_init();
  SCOPE_EXIT(temp_allocator.al_destroy());

  // FILE(LINE) for visual studio click to go to location.
  int log_len = 0;
  const char* log_prefix_format = "%s(%d): %s: ";
  const char* level_str = gc_log_level_strings[(int)level];
  int prefix_len = snprintf(NULL, 0, log_prefix_format, file, line, level_str);
  char* log_buffer = (char*)temp_allocator.al_alloc(log_len + 1);
  snprintf(log_buffer, prefix_len + 1, log_prefix_format, file, line, level_str);
  log_len += prefix_len;

  va_list argptr, argptr2;
  va_start(argptr, format);
  va_copy(argptr2, argptr);
  // +1 for new line char.
  int msg_len = vsnprintf(NULL, 0, format, argptr) + 1;
  va_end(argptr);
  temp_allocator.al_realloc(log_buffer, log_len + msg_len + 1);
  va_start(argptr2, format);
  vsnprintf(log_buffer + log_len, msg_len + 1, format, argptr2);
  va_end(argptr2);
  log_len += msg_len;
  log_buffer[log_len - 1] = '\n';
  log_buffer[log_len] = 0;

  if (level == ELOG_LEVEL_FATAL && !debug_is_debugger_attached()) {
    const char* trace_format = "StackTraces:\n%s";
    char trace[MAX_STACK_TRACE_LENGTH];
    debug_get_stack_trace(trace, MAX_STACK_TRACE_LENGTH);
    int trace_len = snprintf(NULL, 0, trace_format, trace);
    temp_allocator.al_realloc(log_buffer, log_len + trace_len + 1);
    snprintf(log_buffer + log_len, trace_len + 1, trace_format, trace);
    log_len += trace_len;
  }
  // Log to stream
  FILE* stream;
  if (level == ELOG_LEVEL_INFO || level == ELOG_LEVEL_DEBUG) {
    stream = stdout;
  } else {
    stream = stderr;
  }
  fprintf(stream, "%s", log_buffer);
#if OS_WIN()
  OutputDebugStringA(log_buffer);
#endif
  g_log_file.f_write(NULL, log_buffer, log_len);
}

bool log_init(const OSChar* log_path) {
  g_log_file.f_init();
  g_log_file.f_open(log_path, EFILE_MODE_APPEND);
  g_log_inited = g_log_file.f_is_valid();
  return g_log_inited;
}

void log_destroy() {
  if (g_log_inited) {
    g_log_file.f_close();
  }
  g_log_inited = false;
}
