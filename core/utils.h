//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

template <typename T>
void maybe_assign(T* t, T v) {
  if (t)
    *t = v;
}

template <typename T>
const T& min(const T& a, const T& b) {
  return a < b ? a : b;
}

template <typename T, Sz N>
Sz static_array_size(const T(&)[N]) {
  return N;
}

template <typename T>
void swap(T* a, T* b) {
  T temp;
  temp = *a;
  *a = *b;
  *b = temp;
}

template <typename T>
struct Scope_exit_t {
  Scope_exit_t(T f) : f(f) {}
  ~Scope_exit_t() {
    f();
  }
  T f;
};

template <typename T>
auto make_scope_exit_(T f) {
  return Scope_exit_t<T>(f);
}

#define M_unused(a) (void)a

#define M_string_join_expanded_(arg1, arg2) arg1 ## arg2
#define M_string_join_(arg1, arg2) M_string_join_expanded_(arg1, arg2)
#define M_scope_exit(code) \
  auto M_string_join_(zz_scope_exit_at_line_, __LINE__) = make_scope_exit_([&]() { \
    code; \
  }); \
  M_unused(M_string_join_(zz_scope_exit_at_line_, __LINE__))
