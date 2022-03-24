//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string.h"

#include "core/string_utils.h"
#include "test/test.h"

template <typename T_string, typename T_char>
void template_cstring_test_() {
  using T = T_char;
  {
    T_string cstr;
    M_test(cstr.m_p == NULL);
    M_test(cstr.m_length == 0);
  }
  {
    T_string cstr(M_c_or_w(T, 'a'));
    M_test(cstr.m_length == 1);
    M_test(cstr.m_p[0] == M_c_or_w(T, 'a'));
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.m_length == 3);
  }
  {
    T_string cstr(M_c_or_w(T, "abc"), 2);
    M_test(cstr.m_length == 2);
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.ends_with(M_c_or_w(T, "c")));
    M_test(cstr.ends_with(M_c_or_w(T, 'c')));
  }
  {
    T_string cstr(M_c_or_w(T, "abc"), 4);
    M_test(cstr.ends_with(M_c_or_w(T, '\0')));
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.equals(M_c_or_w(T, "abc")));
    M_test(cstr.equals(cstr));
    cstr = T_string(M_c_or_w(T, "abc"), 4);
    M_test(!cstr.equals(M_c_or_w(T, "abc")));
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.find_substr(M_c_or_w(T, "c")).m_p);
    M_test(cstr.find_substr(M_c_or_w(T, "c")).m_length);
    M_test(cstr.find_substr(T_string(M_c_or_w(T, "c"), 2)).m_p == NULL);
    M_test(cstr.find_substr(T_string(M_c_or_w(T, "c"), 2)).m_length == 0);
    cstr = T_string(M_c_or_w(T, "abc"), 4);
    M_test(cstr.find_substr(T_string(M_c_or_w(T, "c"), 2)).m_p);
    M_test(cstr.find_substr(T_string(M_c_or_w(T, "c"), 2)).m_length);
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.find_substr(M_c_or_w(T, "a")).m_p == cstr.m_p);
    M_test(cstr.find_substr(M_c_or_w(T, "ab")).m_p == cstr.m_p);
    M_test(cstr.find_substr(M_c_or_w(T, "b")).m_p == cstr.m_p + 1);
    M_test(cstr.find_substr(M_c_or_w(T, '\0')).m_p == NULL);
    cstr = T_string(M_c_or_w(T, "abc"), 4);
    M_test(cstr.find_substr(M_c_or_w(T, '\0')).m_p == cstr.m_p + 3);
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.find_char(M_c_or_w(T, 'a')).m_p == cstr.m_p);
    M_test(cstr.find_char(M_c_or_w(T, 'b')).m_p == cstr.m_p + 1);
    M_test(cstr.find_char(M_c_or_w(T, '\0')).m_p == NULL);
    cstr = T_string(M_c_or_w(T, "abc"), 4);
    M_test(cstr.find_substr(M_c_or_w(T, '\0')).m_p == cstr.m_p + 3);
  }
  {
    T_string cstr(M_c_or_w(T, "abc"));
    M_test(cstr.find_char_reverse(M_c_or_w(T, 'a')).m_p == cstr.m_p);
    M_test(cstr.find_char_reverse(M_c_or_w(T, 'b')).m_p == cstr.m_p + 1);
    M_test(cstr.find_char_reverse('\0').m_p == NULL);
    cstr = T_string(M_c_or_w(T, "abc"), 4);
    M_test(cstr.find_char_reverse(M_c_or_w(T, '\0')).m_p == cstr.m_p + 3);
  }
}

template <typename T_string, typename T_char>
void template_mstring_test_() {
  using T = T_char;
  {
    T buffer[100] = {};
    T_string mstr(buffer, 100);
    mstr.append(M_c_or_w(T, 'a'));
    M_test(mstr.equals(M_c_or_w(T, "a")));
    mstr.append(M_c_or_w(T, "bc"));
    M_test(mstr.equals(M_c_or_w(T, "abc")));
    mstr.copy(M_c_or_w(T, "aaa"));
    M_test(mstr.equals(M_c_or_w(T, "aaa")));
    mstr.replace(M_c_or_w(T, 'a'), M_c_or_w(T, 'b'));
    M_test(mstr.equals(M_c_or_w(T, "bbb")));
  }
}

void string_test() {
  template_cstring_test_<Cstring_t, char>();
  template_mstring_test_<Mstring_t, char>();
#if M_os_is_win()
  template_cstring_test_<Cwstring_t, wchar_t>();
  template_mstring_test_<Mwstring_t, wchar_t>();
#endif
}
