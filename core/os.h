//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

// OS macros
#if defined(_WIN32) || defined(_WIN64)
#  define M_os_win_ 1
#endif

#if defined(__linux__)
#  define M_os_linux_ 1
#endif

#define M_os_is_win() M_os_win_
#define M_os_is_linux() M_os_linux_
