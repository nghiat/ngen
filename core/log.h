//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/build.h"
#include "core/compiler.h"
#include "core/debug.h"
#include "core/types.h"

#include <stdio.h>
#include <string.h>

enum E_log_level_ {
  e_log_level_info = 0,
  e_log_level_debug = 1,
  e_log_level_warning = 2,
  e_log_level_fatal = 3,
  e_log_level_test = 4,
};

extern bool g_is_log_in_testing;

void ng_log_(E_log_level_ level, const char* file, int line, const char* format, ...);

#define M_log_size_ (1024 + M_max_stack_trace_length_)

bool log_init(const Os_char* log_path);
void log_destroy();

#if M_compiler_is_msvc()
#define M_likely(x) (x)
#define M_unlikely(x) (x)
#else
#define M_likely(x) __builtin_expect((x), 1)
#define M_unlikely(x) __builtin_expect((x), 0)
#endif

// See log_level
#define M_logi(format, ...) ng_log_(e_log_level_info, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define M_logd(format, ...) ng_log_(e_log_level_debug, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define M_logw(format, ...) ng_log_(e_log_level_warning, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if M_is_dev()
#  define M_logf(format, ...)                                                \
    {                                                                        \
      ng_log_(e_log_level_fatal, __FILE__, __LINE__, format, ##__VA_ARGS__); \
      if (debug_is_debugger_attached() && !g_is_log_in_testing) {            \
        M_debug_break_debugger();                                            \
      }                                                                      \
    }
#else
#  define M_logf(format, ...) ng_log_(e_log_level_fatal, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#define M_stringify_expanded_(condition) #condition
#define M_stringify_(condition) M_stringify_expanded_(condition)
#define M_stringify_condition_(condition) M_stringify_(condition) " doesn't match"

#define M_check(condition)               \
  if (M_unlikely(!(condition))) {                  \
    M_logf("%s", M_stringify_condition_(condition)); \
  }

#define M_check_log(condition, format, ...) \
  if (M_unlikely(!(condition))) {                     \
    M_logf(format, ##__VA_ARGS__);            \
  }

#define M_logf_return(format, ...) \
  {                              \
    M_logf(format, ##__VA_ARGS__); \
    return;                      \
  }

#define M_logf_return_val(retval, format, ...) \
  {                                          \
    M_logf(format, ##__VA_ARGS__);             \
    return retval;                           \
  }

#define M_check_return(condition)               \
  if (M_unlikely(!(condition))) {                         \
    M_logf_return("%s", M_stringify_condition_(condition)); \
  }

#define M_check_return_val(condition, retval)               \
  if (M_unlikely(!(condition))) {                                     \
    M_logf_return_val(retval, "%s", M_stringify_condition_(condition)); \
  }

#define M_check_return_false(condition) M_check_return_val(condition, false)

#define M_check_log_return(condition, format, ...) \
  if (M_unlikely(!(condition))) {                            \
    M_logf_return(format, ##__VA_ARGS__);            \
  }

#define M_check_log_return_val(condition, retval, format, ...) \
  if (M_unlikely(!(condition))) {                                        \
    M_logf_return_val(retval, format, ##__VA_ARGS__);            \
  }

#define M_unimplemented() M_logf("UNIMPLEMENTED")
