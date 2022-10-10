//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/dynamic_array.h"
#include "core/hash_table.h"
#include "core/linear_allocator.h"
#include "core/log.h"

#include <ctype.h>

Command_line_t::Command_line_t(Allocator_t* allocator) : m_flags(allocator), m_unnamed_args_allocator("Command_line_t default flag allocator"), m_unnamed_args(&m_unnamed_args_allocator) {}

void Command_line_t::destroy() {
  m_flags.destroy();
  m_unnamed_args_allocator.destroy();
}

void Command_line_t::register_flag(const char* short_flag, const char* long_flag, E_value_type value_type) {
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

  Value_t v;
  if (value_type == e_value_type_bool) {
    v = Value_t(false);
  } else {
    v.m_value_type = value_type;
  }

  if (short_flag) {
    char short_flag_c = short_flag[1];
    M_check(short_flag_c <= sc_max_printable_char);
    if (long_flag) {
      m_short_to_long_flag_map[short_flag_c] = long_flag + 2;
    } else {
      M_check_log_return(m_flags.find(short_flag + 1) == nullptr, "Flag already exists");
      m_flags[short_flag + 1] = v;
      m_short_to_long_flag_map[short_flag_c] = short_flag + 1;
    }
  }
  if (long_flag) {
    M_check_log_return(m_flags.find(long_flag + 2) == nullptr, "Flag already exists");
    m_flags[long_flag + 2] = v;
  }
}

bool Command_line_t::parse(int argc, const char* const* argv) {
  Value_t* pending_value = nullptr;
  for (int i = 1; i < argc; ++i) {
    const char* arg = argv[i];
    if (pending_value) {
      pending_value->m_const_string = arg;
      pending_value = nullptr;
      continue;
    }
    int arg_len = strlen(arg);
    Value_t* v = nullptr;
    if (arg_len == 0) {
      m_unnamed_args.append(arg);
      continue;
    } else if (arg_len == 1) {
      m_unnamed_args.append(arg);
      continue;
    } else if (arg_len == 2) {
      if (arg[0] != '-') {
        m_unnamed_args.append(arg);
        continue;
      }

      M_check_log_return_val(arg[1] != '-', false, "-- is an invalid flag");
      M_check_log_return_val(m_short_to_long_flag_map[arg[1]] != nullptr, false, "%s is an unregistered flag", arg);
      v = m_flags.find(m_short_to_long_flag_map[arg[1]]);
    } else {
      if (arg[0] != '-') {
        m_unnamed_args.append(arg);
        continue;
      }
      M_check_log_return_val(arg[1] == '-' && arg_len > 3, false, "Short flag can only have two characters and long flag has to have two - and more than one character after that");
      v = m_flags.find(arg + 2);
      M_check_log_return_val(v, false, "%s is an unregistered flag", arg);
    }

    if (v->m_value_type == e_value_type_bool) {
      v->m_bool = true;
    } else {
      pending_value = v;
    }
  }
  M_check_log_return_val(!pending_value, false, "Flag %s requires a value", argv[argc - 1]);
  return true;
}

Value_t Command_line_t::get_flag_value(const Cstring_t& flag) const {
  Value_t rv;
  Value_t* rv_p;
  M_check_return_val(flag.m_length > 1, rv);
  M_check_return_val(flag.m_p[0] == '-', rv);
  if (flag.m_length == 2) {
    M_check_return_val(flag.m_p[1] != '-', rv);
    rv_p = m_flags.find(m_short_to_long_flag_map[flag.m_p[1]]);
  }
  if (flag.m_length > 2) {
    M_check_return_val(flag.m_p[1] == '-', rv);
    M_check_return_val(flag.m_length > 3, rv);
    rv_p = m_flags.find(flag.m_p + 2);
  }
  M_check_log_return_val(rv_p, rv, "Unregistered flag");
  return *rv_p;
}

const Dynamic_array_t<const char*>& Command_line_t::get_unnamed_args() const {
  return m_unnamed_args;
}
