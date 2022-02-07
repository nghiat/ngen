//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"
#include "core/os.h"

const Os_char* os_str_find_substr(const Os_char* str, const Os_char* substr);
Sz os_str_get_len(const Os_char* str);
bool os_str_compare(const Os_char* s1, const Os_char* s2);

#if M_os_is_win()
const Os_char* os_str_from_utf8_(const char* s, Os_char* buf, int buf_len);
#endif

