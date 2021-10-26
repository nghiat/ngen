//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

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
struct ScopeExit {
  ScopeExit(T f) : f(f) {}
  ~ScopeExit() { f(); }
  T f;
};

#define M_unused(a) (void)a

#define M_string_join_expanded_(arg1, arg2) arg1 ## arg2
#define M_string_join_(arg1, arg2) M_string_join_expanded_(arg1, arg2)
#define M_scope_exit(code) auto M_string_join_(zz_scope_exit_, __LINE__)([&](){code;}); M_unused(M_string_join_(zz_scope_exit_, __LINE__))
