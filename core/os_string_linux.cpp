//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/os_string.h"

#include <string.h>

const char* os_str_find_substr(const char* str, const char* substr) {
  return strstr(str, substr);
}

size_t os_str_get_len(const char* str) {
  return strlen(str);
}

bool os_str_compare(const char* s1, const char* s2) {
  return !strcmp(s1, s2);
}
