//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string_utils.h"

#include <string.h>

template <>
void string_utils_copy<char>(char* dest, const char* src, int dest_len) {
  if (dest_len) {
    strncpy(dest, src, dest_len);
    dest[dest_len - 1] = 0;
  }
}
