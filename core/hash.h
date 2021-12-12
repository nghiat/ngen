//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

Sz fnv1(const U8* key, int len);

template <typename T>
Sz ng_hash(const T& key) {
  return fnv1((U8*)&key, sizeof(T));
}
