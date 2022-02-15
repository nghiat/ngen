//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string.h"

#include "test/test.h"

template <typename T>
const T* c_or_w_(const char* c, const wchar_t* w);

template <>
const char* c_or_w_(const char* c, const wchar_t* w) {
  return c;
}

template <>
const wchar_t* c_or_w_(const char* c, const wchar_t* w) {
  return w;
}

template <typename T>
T c_or_w_(char c, wchar_t w);

template <>
char c_or_w_(char c, wchar_t w) {
  return c;
}

template <>
wchar_t c_or_w_(char c, wchar_t w) {
  return w;
}

#define M_c_or_w_(str, type) c_or_w_<type>(str, L##str)

template <typename T_string, typename T_char>
void template_cstring_test_() {
  using T = T_char;
  {
    T_string cstr;
    M_test(cstr.m_p == NULL);
    M_test(cstr.m_length == 0);
  }
  {
    T_string cstr(M_c_or_w_('a', T));
    M_test(cstr.m_length == 1);
    M_test(cstr.m_p[0] == M_c_or_w_('a', T));
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.m_length == 3);
  }
  {
    T_string cstr(M_c_or_w_("abc", T), 2);
    M_test(cstr.m_length == 2);
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.ends_with(M_c_or_w_("c", T)));
    M_test(cstr.ends_with(M_c_or_w_('c', T)));
  }
  {
    T_string cstr(M_c_or_w_("abc", T), 4);
    M_test(cstr.ends_with(M_c_or_w_('\0', T)));
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.equals(M_c_or_w_("abc", T)));
    M_test(cstr.equals(cstr));
    cstr = T_string(M_c_or_w_("abc", T), 4);
    M_test(!cstr.equals(M_c_or_w_("abc", T)));
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.find_substr(M_c_or_w_("c", T)).m_p);
    M_test(cstr.find_substr(M_c_or_w_("c", T)).m_length);
    M_test(cstr.find_substr(T_string(M_c_or_w_("c", T), 2)).m_p == NULL);
    M_test(cstr.find_substr(T_string(M_c_or_w_("c", T), 2)).m_length == 0);
    cstr = T_string(M_c_or_w_("abc", T), 4);
    M_test(cstr.find_substr(T_string(M_c_or_w_("c", T), 2)).m_p);
    M_test(cstr.find_substr(T_string(M_c_or_w_("c", T), 2)).m_length);
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.find_substr(M_c_or_w_("a", T)).m_p == cstr.m_p);
    M_test(cstr.find_substr(M_c_or_w_("ab", T)).m_p == cstr.m_p);
    M_test(cstr.find_substr(M_c_or_w_("b", T)).m_p == cstr.m_p + 1);
    M_test(cstr.find_substr(M_c_or_w_('\0', T)).m_p == NULL);
    cstr = T_string(M_c_or_w_("abc", T), 4);
    M_test(cstr.find_substr(M_c_or_w_('\0', T)).m_p == cstr.m_p + 3);
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.find_char(M_c_or_w_('a', T)).m_p == cstr.m_p);
    M_test(cstr.find_char(M_c_or_w_('b', T)).m_p == cstr.m_p + 1);
    M_test(cstr.find_char(M_c_or_w_('\0', T)).m_p == NULL);
    cstr = T_string(M_c_or_w_("abc", T), 4);
    M_test(cstr.find_substr(M_c_or_w_('\0', T)).m_p == cstr.m_p + 3);
  }
  {
    T_string cstr(M_c_or_w_("abc", T));
    M_test(cstr.find_char_reverse(M_c_or_w_('a', T)).m_p == cstr.m_p);
    M_test(cstr.find_char_reverse(M_c_or_w_('b', T)).m_p == cstr.m_p + 1);
    M_test(cstr.find_char_reverse('\0').m_p == NULL);
    cstr = T_string(M_c_or_w_("abc", T), 4);
    M_test(cstr.find_char_reverse(M_c_or_w_('\0', T)).m_p == cstr.m_p + 3);
  }
}

template <typename T_string, typename T_char>
void template_mstring_test_() {
  using T = T_char;
  {
    T buffer[100] = {};
    T_string mstr(buffer, 100);
    mstr.append(M_c_or_w_('a', T));
    M_test(mstr.equals(M_c_or_w_("a", T)));
    mstr.append(M_c_or_w_("bc", T));
    M_test(mstr.equals(M_c_or_w_("abc", T)));
    mstr.copy(M_c_or_w_("aaa", T));
    M_test(mstr.equals(M_c_or_w_("aaa", T)));
    mstr.replace(M_c_or_w_('a', T), M_c_or_w_('b', T));
    M_test(mstr.equals(M_c_or_w_("bbb", T)));
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
