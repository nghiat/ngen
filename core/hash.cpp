//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/hash.h"

#include <string.h>

Sz fnv1(const U8* key, int len) {
  Sz hash = 0xcbf29ce484222325;
  for (int i = 0; i < len; ++i) {
    hash = hash * 1099511628211;
    hash = hash ^ key[i];
  }
  return hash;
}

Sz Hash_t<int>::operator()(const int& key) const {
  return key;
}

Sz Hash_t<const char*>::operator()(const char* const& key) const {
  return fnv1((const U8*)key, strlen(key));
}
