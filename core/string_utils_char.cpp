//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string_utils.h"

#include "core/allocator.h"

#include <stdarg.h>
#include <stdio.h>

template <>
Mutable_string_t_<char> string_format(Allocator_t* allocator, const char* format, ...) {
  va_list argptr;
  va_start(argptr, format);
  va_list argptr2;
  va_copy(argptr2, argptr);
  int len = vsnprintf(NULL, 0, format, argptr);
  va_end(argptr);
  char* str = (char*)allocator->alloc(len + 1);
  vsnprintf(str, len, format, argptr2);
  va_end(argptr2);
  return Mutable_string_t_<char>(str, len + 1);
}
