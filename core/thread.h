//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"

#if OS_WIN()
#include "core/windows_lite.h"
typedef HANDLE ngThreadHandle;

#elif OS_LINUX()
#include <pthread.h>
typedef pthread_t ngThreadHandle;

#else
#error "?"
#endif

typedef void (*ngThreadFunc)(void*);

class ngThread {
public:
  bool thread_init(ngThreadFunc start_func, void* args);
  void thread_wait_for();
  static int thread_get_nums();

  ngThreadHandle m_handle;
  ngThreadFunc m_start_func;
  void* m_args;
};
