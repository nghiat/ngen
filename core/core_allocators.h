//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

class Allocator_t;

// Allocate once and will never change.
extern Allocator_t* g_persistent_allocator;

// General purpose allocator.
extern Allocator_t* g_general_allocator;

bool core_allocators_init();
