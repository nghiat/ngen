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

template <typename T, SZ N>
SZ static_array_size(const T(&)[N]) {
  return N;
}

template <typename T>
struct ScopeExit {
  ScopeExit(T f) : f(f) {}
  ~ScopeExit() { f(); }
  T f;
};

#define UNUSED(a) (void)a

#define STRING_JOIN_EXPANDED_(arg1, arg2) arg1 ## arg2
#define STRING_JOIN_(arg1, arg2) STRING_JOIN_EXPANDED_(arg1, arg2)
#define SCOPE_EXIT(code) auto STRING_JOIN_(zz_scope_exit_, __LINE__)([&](){code;}); UNUSED(STRING_JOIN_(zz_scope_exit_, __LINE__))
