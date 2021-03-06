//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/dynamic_lib.h"

#include <dlfcn.h>

bool Dynamic_lib_t::open(const char* name) {
  m_handle = dlopen(name, RTLD_LAZY | RTLD_LOCAL);
  return m_handle != nullptr;
}

void Dynamic_lib_t::close() {
  dlclose(m_handle);
}

void* Dynamic_lib_t::get_proc(const char* name) {
  return dlsym(m_handle, name);
}
