//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

#if OS_WIN()

#  define FORWARD_DECLARE_HANDLE(name) \
    struct name##__;                      \
    typedef struct name##__* name

FORWARD_DECLARE_HANDLE(HDC);
FORWARD_DECLARE_HANDLE(HGLRC);
FORWARD_DECLARE_HANDLE(HINSTANCE);
FORWARD_DECLARE_HANDLE(HWND);

#  undef FORWARD_DECLARE_HANDLE

typedef void* HANDLE;
typedef HINSTANCE HMODULE;

#endif // OS_WIN()
