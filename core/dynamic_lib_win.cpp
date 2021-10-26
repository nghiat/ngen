//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/dynamic_lib.h"

#include <Windows.h>

bool Dynamic_lib::dl_open(const char* name) {
  m_handle = (void*)LoadLibraryA(name);
  return m_handle != NULL;
}

void Dynamic_lib::dl_close() {
  FreeLibrary((HMODULE)m_handle);
}

void* Dynamic_lib::dl_get_proc(const char* name) {
  return (void*)GetProcAddress((HMODULE)m_handle, name);
}
