//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/os_string.h"

#include <wchar.h>

const wchar_t* os_str_find_substr(const wchar_t* str, const wchar_t* substr) {
  return wcsstr(str, substr);
}

size_t os_str_get_len(const wchar_t* str) {
  return wcslen(str);
}

bool os_str_compare(const wchar_t* s1, const wchar_t* s2) {
  return !wcscmp(s1, s2);
}
