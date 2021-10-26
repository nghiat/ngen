//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#if defined(__clang__)
#define M_compiler_clang_ 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define M_compiler_gcc_ 1
#elif defined(_MSC_VER)
#define M_compiler_msvc_ 1
#endif

#define M_is_clang() M_compiler_clang_
