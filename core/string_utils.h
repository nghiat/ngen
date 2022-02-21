//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/string.h"

class Allocator_t;

template <typename T>
Mutable_string_t_<T> string_format(Allocator_t* allocator, const T* format, ...);
