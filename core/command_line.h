//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/allocator.h"
#include "core/dynamic_array.h"
#include "core/hash_table.h"
#include "core/linear_allocator.h"
#include "core/value.h"

class Command_line {
public:
  Command_line();
  struct Flag_ {
    const char* short_flag;
    const char* long_flag;
  };
  bool init(Allocator* allocator);
  void destroy();
  // |short_flag| must only be a digit or a letter.
  // |long_flag| length must be > 1.
  // One of those two has to be not NULL.
  // If |value_type| is e_value_type_bool, then its default value is false and the flag will toggle the value.
  void add_flag(const char* short_flag, const char* long_flag, E_value_type value_type);
  void parse(int argc, const char** argv);
  Value get_flag_value(const char* flag) const;
  Value get_default_value() const;

// private:
  static const U8 sc_max_printable_char = 'z';
  const char* m_short_to_long_flag_map[sc_max_printable_char] = {};
  Hash_map<const char*, Value> m_flags;
  Linear_allocator<128> m_default_flag_allocator;
  Dynamic_array<const char*> m_default_flag_values;
};
