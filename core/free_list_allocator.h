//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/allocator.h"

#include "core/ng_types.h"

struct FreeBlock;

/// An allocator that keeps a linked list of blocks of available spaces.
/// When you request an allocation, it finds the smallest block that can keeps
/// the size of the allcation then shrinks that block. When you request a
/// freeation, it creates a new blocks and merges with nearby blocks if
/// they are contiguous.
class FreeListAllocator : public ngAllocator {
public:
  FreeListAllocator(const char* name, SZ total_size) : ngAllocator(name, total_size) {}
  bool fla_init();
  void al_destroy() override;
  void* al_aligned_alloc(SIP size, SIP alignment) override;
  void* al_realloc(void* p, SIP size) override;
  void al_free(void* p) override;

private:
  // Finds the smallest possible block that can fits the |requiredSize| with
  // |alignment| and returns the pointer to the beginning of the allocation
  // region.
  U8* find_best_fit_free_block(FreeBlock** o_fit_block, FreeBlock** o_prior_block, SIP size, SIP alignment);
  // Get the free_block before and after |p|.
  void get_adjacent_blocks(FreeBlock** prior_block, FreeBlock** next_block, U8* p);
  // Get the free_block before and after |p|.
  // Links 2 consecutive separated blocks. If the |priorBlock| is null,
  // set |first_free_block_| to |block|.
  void link_separated_blocks(FreeBlock* prior_block, FreeBlock* block);
  // Link two blocks together. If they are contiguous, merge them
  // and assign *second to *first.
  void link_and_merge_free_blocks(FreeBlock** first, FreeBlock** second);
  // Add a FreeBlock to the FreeBlock linked list.
  // If the FreeBlock is adjacent to the prior block or the next block, they
  // will be merged
  void add_and_merge_free_block(FreeBlock* block);
  // Shrink |block| (remove |size| on the left) and link the new |FreeBlock|
  // with |prior_block| if it is possible. Returns true if the block can be
  // shrunk (means that there is enough space for at least a AllocHeader
  // after shrinking), false otherwise.
  bool shrink_free_block(FreeBlock* block, FreeBlock* prior_block, SIP size);
  void realloc_smaller(U8* p, SIP size, FreeBlock* prior_block, FreeBlock* next_block);
  U8* realloc_bigger(U8* p, SIP size, FreeBlock* prior_block, FreeBlock* next_block);

  U8* m_start;
  FreeBlock* m_first_block;
};
