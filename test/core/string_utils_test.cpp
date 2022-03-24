//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string_utils.h"

#include "core/hash_table.inl"
#include "core/linear_allocator.inl"
#include "core/utils.h"
#include "test/test.h"

template <typename T>
void template_string_utils_test() {
  // No braces
  {
    const T* format = M_c_or_w(T, "abc");
    Linear_allocator_t<> allocator("string_utils_test_allocator");
    allocator.init();
    M_scope_exit(allocator.destroy());
    int brace_pair_count = 1;
    auto dict = string_format_setup<T>(&allocator, format, &brace_pair_count);
    M_test(brace_pair_count == 0);

    auto rv = string_format<T>(&allocator, format, dict);
    M_test(rv.m_length == 3);
    M_test(rv.equals(M_c_or_w(T, "abc")));
  }

  // Duplicated braces and brace with new line
  // {
  //   const T* format = M_c_or_w(T, "{{}} {\n}\n");
  //   Linear_allocator_t<> allocator("string_utils_test_allocator");
  //   allocator.init();
  //   M_scope_exit(allocator.destroy());
  //   int brace_pair_count = 1;
  //   auto dict = string_format_setup<T>(&allocator, format, &brace_pair_count);
  //   M_test(brace_pair_count == 0);

  //   auto rv = string_format<T>(&allocator, format, dict);
  //   M_test(rv.m_length == 3);
  //   M_test(rv.equals(M_c_or_w(T, "abc")));
  // }

  // One identifier
  {
    const T* format = M_c_or_w(T, "{{id1}}");
    Linear_allocator_t<> allocator("string_utils_test_allocator");
    allocator.init();
    M_scope_exit(allocator.destroy());
    int brace_pair_count = 0;
    auto dict = string_format_setup<T>(&allocator, format, &brace_pair_count);
    M_test(brace_pair_count == 1);

    auto rv = string_format<T>(&allocator, format, dict);
    M_test(rv.m_length == 0);
    dict[M_c_or_w(T, "id1")] = M_c_or_w(T, "abc");
    rv = string_format<T>(&allocator, format, dict);
    M_test(rv.equals(M_c_or_w(T, "abc")));
  }

  // Multiple identifiers
  {
    const T* format = M_c_or_w(T, "{{id1}} {{id1}} {{id2}} aaa");
    Linear_allocator_t<> allocator("string_utils_test_allocator");
    allocator.init();
    M_scope_exit(allocator.destroy());
    int brace_pair_count = 0;
    auto dict = string_format_setup<T>(&allocator, format, &brace_pair_count);
    M_test(brace_pair_count == 3);

    auto rv = string_format<T>(&allocator, format, dict);
    M_test(rv.m_length == 0);
    dict[M_c_or_w(T, "id1")] = M_c_or_w(T, "abc");
    rv = string_format<T>(&allocator, format, dict);
    M_test(rv.m_length == 0);
    dict[M_c_or_w(T, "id2")] = M_c_or_w(T, "def");
    rv = string_format<T>(&allocator, format, dict);
    M_test(rv.equals(M_c_or_w(T, "abc abc def aaa")));
  }
}

void string_utils_test() {
  template_string_utils_test<char>();
  template_string_utils_test<wchar_t>();
}
