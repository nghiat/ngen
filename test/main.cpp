//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/hash_table.inl"

Hash_map<const char*, void (*)()> g_tests_;

#define M_register_test(func) \
  extern void func(); \
  g_tests_[#func] = func

int main(int argc, const char** argv) {
  core_init(M_os_txt("test.log"));
  Command_line cl;
  cl.init(g_persistent_allocator);
  cl.add_flag(nullptr, "--filter", e_value_type_string);
  cl.parse(argc, argv);

  g_tests_.init(g_persistent_allocator);
  M_register_test(dynamic_array_test);
  return 0;
}
