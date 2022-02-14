//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

Sz fnv1(const U8* key, int len);

template <typename T>
struct Hash_t {
  Sz operator()(const T& key) const {
    return fnv1((U8*)&key, sizeof(T));
  }
};

template <>
struct Hash_t<int> {
  Sz operator()(const int& key) const;
};

template <>
struct Hash_t<const char*> {
  Sz operator()(const char* const& key) const;
};
