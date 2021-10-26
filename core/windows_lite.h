//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

#if M_os_is_win()

#define M_forward_declare_handle_(name) \
    struct name##__;                  \
    typedef struct name##__* name

M_forward_declare_handle_(HDC);
M_forward_declare_handle_(HGLRC);
M_forward_declare_handle_(HINSTANCE);
M_forward_declare_handle_(HWND);

#undef M_forward_declare_handle_

typedef void* HANDLE;
typedef HINSTANCE HMODULE;

#endif // M_os_is_win()
