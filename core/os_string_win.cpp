//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/os_string.h"

#include "core/log.h"

#include <wchar.h>

#include <Windows.h>

const wchar_t* os_str_find_substr(const wchar_t* str, const wchar_t* substr) {
  return wcsstr(str, substr);
}

size_t os_str_get_len(const wchar_t* str) {
  return wcslen(str);
}

bool os_str_compare(const wchar_t* s1, const wchar_t* s2) {
  return !wcscmp(s1, s2);
}

const wchar_t* os_str_from_utf8_(const char* s, wchar_t* buf, int buf_len) {
  int output_len = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL,0);
  M_check_return_val(output_len <= buf_len, NULL);
  MultiByteToWideChar(CP_UTF8, 0, s, -1, buf, buf_len);
  return buf;
}
