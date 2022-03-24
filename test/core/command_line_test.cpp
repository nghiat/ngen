//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/command_line.h"

#include "core/linear_allocator.h"
#include "core/utils.h"
#include "test/test.h"

void command_line_test() {
  Linear_allocator_t<> allocator("string_utils_test_allocator");
  allocator.init();
  M_scope_exit(allocator.destroy());

  // Unregisted short flag
  {
    Command_line_t cl;
    M_scope_exit(cl.destroy());
    cl.init(&allocator);
    const char* args[] = {
      "dummy",
      "-a",
    };
    M_test(cl.parse(2, args) == false);
  }
  // Unregisted long flag
  {
    Command_line_t cl;
    M_scope_exit(cl.destroy());
    cl.init(&allocator);
    const char* args[] = {
      "dummy",
      "--aa",
    };
    M_test(cl.parse(2, args) == false);
  }
  // Test bool value
  {
    Command_line_t cl;
    M_scope_exit(cl.destroy());
    cl.init(&allocator);
    cl.register_flag("-a", "--aa", e_value_type_bool);
    const char* args[] = {
      "dummy",
      "--aa",
    };
    M_test(cl.parse(2, args));
    M_test(cl.get_flag_value("-a").get_bool());
    M_test(cl.get_flag_value("--aa").get_bool());
  }
  // Test string value
  {
    Command_line_t cl;
    M_scope_exit(cl.destroy());
    cl.init(&allocator);
    cl.register_flag("-a", "--aa", e_value_type_string);
    const char* args[] = {
      "dummy",
      "--aa",
    };
    M_test(cl.parse(2, args) == false);
    const char* args2[] = {
      "dummy",
      "--aa",
      "-a",
    };
    M_test(cl.parse(3, args2) == true);
    M_test(Cstring_t("-a").equals(cl.get_flag_value("-a").get_string()));
    M_test(Cstring_t("-a").equals(cl.get_flag_value("--aa").get_string()));
  }
}
