//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#if defined(__clang__)
#define COMPILER_CLANG_ 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define COMPILER_GCC_ 1
#elif defined(_MSC_VER)
#define COMPILER_MSVC_ 1
#endif

#define IS_CLANG() COMPILER_CLANG_
