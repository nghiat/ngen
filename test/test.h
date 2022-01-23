//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/hash_table.h"
#include "core/log.h"

extern Hash_map<const char*, void (*)()> g_tests_;
// TODO: atomic?
extern int g_total_test_count_;
extern int g_passed_test_count_;

#define M_register_test(func) \
  extern void func(); \
  g_tests_[#func] = func

#define M_test(condition)                                                                        \
  ++g_total_test_count_;                                                                         \
  if (M_unlikely(!(condition))) {                                                                \
    ng_log_(e_log_level_test, __FILE__, __LINE__, "Test (%s) failed", M_stringify_(condition)); \
    if (debug_is_debugger_attached()) {                                                          \
      M_debug_break_debugger();                                                                  \
    }                                                                                            \
  } else {                                                                                       \
    ++g_passed_test_count_;                                                                      \
  }
