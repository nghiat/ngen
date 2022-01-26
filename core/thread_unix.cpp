//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/thread.h"

#include "core/log.h"

#include <pthread.h>
#include <unistd.h>

static void* platform_thread_start(void* args) {
  Thread_t* thread = (Thread_t*)args;
  thread->m_start_func(thread->m_args);
  return NULL;
}

bool Thread_t::init(ngThread_func start_func, void* args) {
  m_start_func = start_func;
  m_args = args;
  M_check_log_return_val(pthread_create(&m_handle, NULL, platform_thread_start, (void*)this) == 0, false, "Can't create a new thread");
  return true;
}

void Thread_t::wait_for() {
  void* res;
  pthread_join(m_handle, &res);
}

int Thread_t::get_total_thread_count() {
  return sysconf(_SC_NPROCESSORS_ONLN);
}
