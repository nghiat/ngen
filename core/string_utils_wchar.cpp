//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string_utils.h"

#include "core/log.h"

#include <wchar.h>

template <>
void string_utils_copy<wchar_t>(wchar_t* dest, const wchar_t* src, int dest_len) {
  if (dest_len) {
    wcsncpy(dest, src, dest_len);
    dest[dest_len - 1] = 0;
  }
}
