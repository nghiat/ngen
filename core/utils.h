//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

template <typename T>
void maybe_assign(T* t, T v) {
  if (t) {
    *t = v;
  }
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
  char temp[sizeof(T)];
  *(T*)temp = *a;
  *a = *b;
  *b = *(T*)temp;
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

constexpr U32 four_cc(const char cc[4]) {
  return cc[0] | cc[1] << 8 | cc[2] << 16 | cc[3] << 24;
}

#define M_unused(a) (void)a

#define M_string_join_expanded_(arg1, arg2) arg1 ## arg2
#define M_string_join_(arg1, arg2) M_string_join_expanded_(arg1, arg2)
#define M_scope_exit_lambda(lambda) \
  auto M_string_join_(zz_scope_exit_at_line_, __LINE__) = make_scope_exit_(lambda); \
  M_unused(M_string_join_(zz_scope_exit_at_line_, __LINE__))
#define M_scope_exit(code) \
  M_scope_exit_lambda([&]() {\
      code; \
  })
