//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/build.h"
#include "core/debug.h"
#include "core/ng_types.h"

#include <stdio.h>
#include <string.h>

enum ELogLevel {
  ELOG_LEVEL_INFO = 0,
  ELOG_LEVEL_DEBUG = 1,
  ELOG_LEVEL_WARNING = 2,
  ELOG_LEVEL_FATAL = 3,
};

#if IS_DEV()
void log_internal(ELogLevel level, const char* file, int line, const char* format, ...);
#else
#define log_internal(..)
#endif

#define LOG_SIZE (1024 + MAX_STACK_TRACE_LENGTH)

bool log_init(const OSChar* log_path);
void log_destroy();

// See log_level
#define LOGI(format, ...) log_internal(ELOG_LEVEL_INFO, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOGD(format, ...) log_internal(ELOG_LEVEL_DEBUG, __FILE__, __LINE__, format, ##__VA_ARGS__)
#define LOGW(format, ...) log_internal(ELOG_LEVEL_WARNING, __FILE__, __LINE__, format, ##__VA_ARGS__)

#if IS_DEV()
#  define LOGF(format, ...)                                                        \
    {                                                                                 \
      log_internal(ELOG_LEVEL_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__); \
      if (debug_is_debugger_attached()) {                                          \
        DEBUG_BREAK_DEBUGGER();                                                    \
      }                                                                               \
    }
#else
#  define LOGF(format, ...) log_internal(ELOG_LEVEL_FATAL, __FILE__, __LINE__, format, ##__VA_ARGS__)
#endif

#define STRINGIFY_EXPANDED_(condition) #condition
#define STRINGIFY_(condition) STRINGIFY_EXPANDED_(condition) " doesn't match"

#define CHECK(condition) \
  if (!(condition))         \
    LOGF("%s", STRINGIFY_(condition));

#define CHECK_LOG(condition, format, ...) \
  if (!(condition))                          \
    LOGF(format, __VA_ARGS__);

#define LOGF_RETURN(format, ...) \
  {                                 \
    LOGF(format, ##__VA_ARGS__); \
    return;                         \
  }

#define LOGF_RETURN_VAL(retval, format, ...) \
  {                                             \
    LOGF(format, ##__VA_ARGS__);             \
    return retval;                              \
  }

#define CHECK_RETURN(condition) \
  if (!(condition))                \
    LOGF_RETURN("%s", STRINGIFY_(condition));

#define CHECK_RETURN_VAL(condition, retval) \
  if (!(condition))                            \
    LOGF_RETURN_VAL(retval, "%s", STRINGIFY_(condition));

#define CHECK_LOG_RETURN(condition, format, ...) \
  if (!(condition))                                 \
    LOGF_RETURN(format, __VA_ARGS__);

#define CHECK_LOG_RETURN_VAL(condition, retval, format, ...) \
  if (!(condition))                                             \
    LOGF_RETURN_VAL(retval, format, __VA_ARGS__);
