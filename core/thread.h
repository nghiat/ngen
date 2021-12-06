//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

#if M_os_is_win()
#include "core/windows_lite.h"
typedef HANDLE ngThread_handle_;

#elif M_os_is_linux()
#include <pthread.h>
typedef pthread_t ngThread_handle_;

#else
#error "?"
#endif

typedef void (*ngThread_func)(void*);

class ngThread {
public:
  bool init(ngThread_func start_func, void* args);
  void wait_for();
  static int get_nums();

  ngThread_handle_ m_handle;
  ngThread_func m_start_func;
  void* m_args;
};
