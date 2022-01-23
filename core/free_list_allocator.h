//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/allocator.h"

#include "core/ng_types.h"

struct Free_block_;

/// An allocator that keeps a linked list of blocks of available spaces.
/// When you request an allocation, it finds the smallest block that can keeps
/// the size of the allcation then shrinks that block. When you request a
/// freeation, it creates a new blocks and merges with nearby blocks if
/// they are contiguous.
class FreeList_allocator : public Allocator {
public:
  FreeList_allocator(const char* name, Sz total_size) : Allocator(name, total_size) {}
  bool init();
  void destroy() override;
  void* aligned_alloc(Sip size, Sip alignment) override;
  void* realloc(void* p, Sip size) override;
  void free(void* p) override;

  U8* m_start;
  Free_block_* m_first_block;

private:
  // Finds the smallest possible block that can fits the |requiredSize| with
  // |alignment| and returns the pointer to the beginning of the allocation
  // region.
  U8* find_best_fit_free_block_(Free_block_** o_fit_block, Free_block_** o_prior_block, Sip size, Sip alignment);
  // Get the free_block before and after |p|.
  void get_adjacent_blocks_(Free_block_** prior_block, Free_block_** next_block, U8* p);
  // Get the free_block before and after |p|.
  // Links 2 consecutive separated blocks. If the |priorBlock| is null,
  // set |first_free_block_| to |block|.
  void link_separated_blocks_(Free_block_* prior_block, Free_block_* block);
  // Link two blocks together. If they are contiguous, merge them
  // and assign *second to *first.
  void link_and_merge_free_blocks_(Free_block_** first, Free_block_** second);
  // Add a Free_block_ to the Free_block_ linked list.
  // If the Free_block_ is adjacent to the prior block or the next block, they
  // will be merged
  void add_and_merge_free_block_(Free_block_* block);
  // Shrink |block| (remove |size| on the left) and link the new |Free_block_|
  // with |prior_block| if it is possible. Returns true if the block can be
  // shrunk (means that there is enough space for at least a Alloc_header_
  // after shrinking), false otherwise.
  bool shrink_free_block_(Free_block_* block, Free_block_* prior_block, Sip size);
  void realloc_smaller_(U8* p, Sip size, Free_block_* prior_block, Free_block_* next_block);
  U8* realloc_bigger_(U8* p, Sip size, Free_block_* prior_block, Free_block_* next_block);
};
