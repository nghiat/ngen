//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/build.h"
#include "core/ng_types.h"

/// Here is the sketch for a general allocation.
/// start                    p         size        end
///  |                       |----------------------|
///  o_______________________o______________________o
///  |  |  allocation_header | allocation | padding |
///   o                                       |
///   |                                       o
/// prefix padding                        suffix padding (the remaning space
///  (alignment)                          is not enough for another
///                                       allocation_header)
/// An allocation_header is placed before any pointer returned by alloc()
/// or aligned_alloc() to keep track of the allocation (prefix padding is
/// used for alignment). In a case when space between the current allocation
/// and the next allocation is not enough for any allocation then the current
/// allocation will acquire the remaining space (suffix padding).
struct AllocHeader {
  U8* start;
  SIP size;
  SIP alignment;
#if IS_DEV()
  U8* p;
#endif
};

AllocHeader* get_allocation_header(void* p);
U8* align_forward(U8* p, SIP alignment);
bool check_aligned_alloc(SIP size, SIP alignment);
bool check_p_in_dev(void* p);
