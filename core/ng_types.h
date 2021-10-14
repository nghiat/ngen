//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
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

typedef uintptr_t UIP;
typedef intptr_t SIP;

typedef float F32;
typedef double F64;

typedef size_t SZ;

#if OS_WIN()
#  define OS_TXT(x) L##x
#  define OS_TXT_PR "%ls"
typedef wchar_t OSChar;

#elif OS_LINUX()
#  define OS_TXT(x) x
#  define OS_TXT_PR "%s"
typedef char OSChar;

#else
#error "?"
#endif

