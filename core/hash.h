//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

Sz fnv1(const U8* key, int len);

template <typename T>
struct Ng_hash {
  Sz operator()(const T& key) const {
    return fnv1((U8*)&key, sizeof(T));
  }
};

template <>
struct Ng_hash<int> {
  Sz operator()(const int& key) const;
};

template <>
struct Ng_hash<const char*> {
  Sz operator()(const char* const& key) const;
};
