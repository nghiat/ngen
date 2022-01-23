//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"
#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/hash_table.inl"
#include "test/test.h"

Hash_map<const char*, void (*)()> g_tests_;
int g_total_test_count_ = 0;
int g_passed_test_count_ = 0;

int main(int argc, const char** argv) {
  g_is_log_in_testing = true;
  core_init(M_os_txt("test.log"));
  Command_line cl;
  cl.init(g_persistent_allocator);
  cl.parse(argc, argv);

  g_tests_.init(g_persistent_allocator);
  M_register_test(linear_allocator_test);
  M_register_test(hash_map_test);
  M_register_test(utils_test);
  for (auto& test : g_tests_) {
    M_logi("Running test %s", test.key);
    test.value();
  }

  g_is_log_in_testing = false;
  M_logi("%d/%d tests passed", g_passed_test_count_, g_total_test_count_);
  return 0;
}
