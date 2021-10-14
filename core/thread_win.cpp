//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/thread.h"

#include "core/log.h"

#include <Windows.h>

static DWORD platform_thread_start(void* args) {
  ngThread* thread = (ngThread*)args;
  thread->m_start_func(thread->m_args);
  return 0;
}

bool ngThread::thread_init(ngThreadFunc start_func, void* args) {
  m_start_func = start_func;
  m_args = args;
  m_handle = CreateThread(NULL, 0, platform_thread_start, (void*)this, 0, NULL);
  CHECK_LOG_RETURN_VAL(m_handle != NULL, false, "Can't create a new thread");
  return true;
}

void ngThread::thread_wait_for() {
  WaitForSingleObject(m_handle, INFINITE);
}

int ngThread::thread_get_nums() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}
