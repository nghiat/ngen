//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

// OS macros
#if defined(_WIN32) || defined(_WIN64)
#  define OS_WIN_ 1
#endif

#if defined(__linux__)
#  define OS_LINUX_ 1
#endif

#define OS_WIN() OS_WIN_
#define OS_LINUX() OS_LINUX_
