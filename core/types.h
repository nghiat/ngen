//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

#include <stddef.h>
#include <stdint.h>

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef uintptr_t Uip; // unsigned integer pointer
typedef intptr_t Sip; // signed integer pointer

typedef float F32;
typedef double F64;

typedef size_t Sz;

#if M_os_is_win()
#  define M_os_txt(x) L##x
#  define M_os_txt_pr "%ls"
typedef wchar_t Os_char;

#elif M_os_is_linux()
#  define M_os_txt(x) x
#  define M_os_txt_pr "%s"
typedef char Os_char;

#else
#error "?"
#endif

