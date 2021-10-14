//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

class ngAllocator;

// Allocate once and will never change.
extern ngAllocator* g_persistent_allocator;

// General purpose allocator.
extern ngAllocator* g_general_allocator;

bool core_allocators_init();
