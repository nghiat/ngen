//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/dynamic_array.inl"
#include "core/hash_table.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"

#include <ctype.h>

bool Command_line::init(Allocator* allocator) {
  bool rv = m_flags.init(allocator);
  rv &= m_default_flag_allocator.init();
  rv &= m_default_flag_values.init(&m_default_flag_allocator);
  return rv;
}

void Command_line::destroy() {
  m_flags.destroy();
  m_default_flag_allocator.destroy();
}

void Command_line::add_flag(const char* short_flag, const char* long_flag, E_value_type value_type) {
  M_check_return(short_flag || long_flag);
  if (short_flag) {
    M_check_log_return(strlen(short_flag) == 2 && short_flag[0] == '-' && isdigit(short_flag[1]) || isalpha(short_flag[1]), "Invalid short_flag");
  }
  if (long_flag) {
    int long_flag_len = strlen(long_flag);
    M_check_log_return(long_flag_len > 3 && long_flag[0] == '-' && long_flag[1] == '-', "Invalid long_flag");
    M_check_log_return(isdigit(long_flag[2]) || isalpha(long_flag[2]), "Invalid long_flag");
    for (int i = 3; i < long_flag_len; ++i) {
      char c = long_flag[i];
      M_check_log_return(isdigit(c) || isalpha(c) || c == '-', "Invalid long_flag");
    }
  }

  Value v;
  if (value_type == e_value_type_bool) {
    v = Value(false);
  }

  if (short_flag) {
    char short_flag_c = short_flag[1];
    M_check(short_flag_c <= sc_max_printable_char);
    if (long_flag) {
      m_short_to_long_flag_map[short_flag_c] = long_flag + 2;
    } else {
      m_short_to_long_flag_map[short_flag_c] = short_flag + 1;
      // TODO check if exists
      m_flags[short_flag + 1] = v;
    }
  }
  if (long_flag) {
    // TODO check if exists
    m_flags[long_flag + 2] = v;
  }
}

void Command_line::parse(int argc, const char** argv) {
  Value* pending_value = nullptr;
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (pending_value) {
      pending_value->m_const_string = arg;
      pending_value = nullptr;
      continue;
    }
    int arg_len = strlen(arg);
    Value* v = nullptr;
    if (arg_len == 0) {
      m_default_flag_values.append(arg);
      continue;
    } else if (arg_len == 1) {
      m_default_flag_values.append(arg);
      continue;
    } else if (arg_len == 2) {
      if (arg[0] != '-') {
        m_default_flag_values.append(arg);
        continue;
      }

      M_check_log_return(arg[1] != '-', "Invalid flag");
      M_check_log_return(m_short_to_long_flag_map[arg[1]] != nullptr, "%s is an invalid flag", arg);
      v = m_flags.find(m_short_to_long_flag_map[arg[1]]);
    } else {
      if (arg[0] != '-') {
        m_default_flag_values.append(arg);
        continue;
      }
      M_check_log_return(arg[1] == '-' && arg_len > 3, "Invalid flag");
      v = m_flags.find(arg + 2);
    }

    if (v->m_value_type == e_value_type_bool) {
      v->m_bool = true;
    } else {
      pending_value = v;
    }
  }
}
