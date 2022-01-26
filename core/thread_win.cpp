//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/thread.h"

#include "core/log.h"

#include <Windows.h>

static DWORD platform_thread_start(void* args) {
  Thread_t* thread = (Thread_t*)args;
  thread->m_start_func(thread->m_args);
  return 0;
}

bool Thread_t::init(ngThread_func start_func, void* args) {
  m_start_func = start_func;
  m_args = args;
  m_handle = CreateThread(NULL, 0, platform_thread_start, (void*)this, 0, NULL);
  M_check_log_return_val(m_handle != NULL, false, "Can't create a new thread");
  return true;
}

void Thread_t::wait_for() {
  WaitForSingleObject(m_handle, INFINITE);
}

int Thread_t::get_total_thread_count() {
  SYSTEM_INFO info;
  GetSystemInfo(&info);
  return info.dwNumberOfProcessors;
}
