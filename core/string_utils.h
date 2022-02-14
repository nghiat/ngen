//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"
#include "core/os.h"

// memory functions
template <typename T>
void string_utils_copy(T* dest, const T* src, int dest_len);

// modification functions
template <typename T>
void string_utils_replace(T* str, const T* substr);
