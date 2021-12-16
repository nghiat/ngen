//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/hash_map.h"
#include "core/value.h"

class Command_line {
public:
  void add_flag(const char* short_flag, const char* long_flag, E_value_type value_type);

// private:
  Hash_map<const char*, Value> m_flags;
};
