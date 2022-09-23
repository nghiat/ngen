//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/string.h"
#include "core/hash_table.h"

class Allocator_t;

template <typename T>
const T* c_or_w_(const char* c, const wchar_t* w);
template <typename T>
T c_or_w_(char c, wchar_t w);
#define M_c_or_w(type, str) c_or_w_<type>(str, L##str)

template <typename T>
Mstring_t_<T> string_printf(Allocator_t* allocator, const T* format, ...);

template <typename T>
Hash_map_t<Cstring_t_<T>, Cstring_t_<T>> string_format_setup(Allocator_t* allocator, const Cstring_t_<T>& format, int* o_brace_pair_count);

template <typename T>
Mstring_t_<T> string_format(Allocator_t* allocator, const Cstring_t_<T>& format, Hash_map_t<Cstring_t_<T>, Cstring_t_<T>>& dict);
