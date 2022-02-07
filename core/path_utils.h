//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"
#include "core/os.h"
#include "core/os_string.h"
#include "core/utils.h"

#define M_max_path_len (256)

bool path_utils_init();
Os_char* path_from_exe_dir(Os_char* out, const Os_char* sub_path, int len);

#if M_os_is_win()
#define M_utf8_to_os_char_path(utf8, out) \
  wchar_t M_string_join_(zz_wchar_path_at_line_, __LINE__)[M_max_path_len]; \
  out = os_str_from_utf8_(utf8, M_string_join_(zz_wchar_path_at_line_, __LINE__), M_max_path_len)

#elif M_os_is_linux()
#define M_utf8_to_os_char_path(utf8, out) out = utf8;

#else
#error "?"
#endif // M_os_is_win()
