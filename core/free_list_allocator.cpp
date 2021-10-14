//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/free_list_allocator.h"

#include "core/allocator_internal.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

// FreeBlock and allocations use a shared memory region.
struct FreeBlock {
  /// |size| includes this struct.
  SIP size;
  FreeBlock* next;
};

static_assert(
    sizeof(AllocHeader) >= sizeof(FreeBlock),
    "We assume that sizeof(AllocHeader) >= sizeof(FreeBlock) so when "
    "freeating an allocation, we always have space for a FreeBlock");

// Returns true if |size| is bigger than sizeof(AllocHeader) which we
// assume that it is bigger than sizeof(FreeBlock).
static bool is_enough_for_allocation_header(SIP size) {
  return size > sizeof(AllocHeader);
}

static bool is_allocaiton_adjacent_to_free_block(AllocHeader* header, FreeBlock* block) {
  return block && (U8*)header + sizeof(AllocHeader) + header->size == (U8*)block;
}

bool FreeListAllocator::fla_init() {
  m_used_size = 0;
  m_start = (U8*)malloc(m_total_size);
  CHECK_LOG_RETURN_VAL(m_start, false, "Can't init allocator \"%s\": Out of memory");
  m_first_block = (FreeBlock*)m_start;
  m_first_block->size = m_total_size;
  m_first_block->next = NULL;
  return true;
}

void FreeListAllocator::al_destroy() {
  if (m_start) {
    ::free(m_start);
  }
}

void* FreeListAllocator::al_aligned_alloc(SIP size, SIP alignment) {
  CHECK_LOG_RETURN_VAL(check_aligned_alloc(size, alignment), NULL, "Alignment is not power of 2");
  FreeBlock* fit_block;
  FreeBlock* prior_block;
  U8* p = find_best_fit_free_block(&fit_block, &prior_block, size, alignment);
  CHECK_LOG_RETURN_VAL(p, NULL, "Free list allocator \"%s\" doesn't have enough space to alloc %d bytes", m_name, size);
  SIP padding_and_header = p - (U8*)fit_block;
  bool rv = shrink_free_block(fit_block, prior_block, padding_and_header + size);
  AllocHeader* hdr = get_allocation_header(p);
  hdr->start = (U8*)fit_block;
  hdr->size = rv ? size : fit_block->size - padding_and_header;
  hdr->alignment = alignment;
#if IS_DEV()
  hdr->p = p;
#endif
  return p;
}

void* FreeListAllocator::al_realloc(void* p, SIP size) {
  CHECK_LOG_RETURN_VAL(check_p_in_dev(p) && size, NULL, "Invalid pointer to realloc");

  AllocHeader* header = get_allocation_header(p);
  // Remaining free space is surely not enough.
  if (size > header->size + (m_total_size - m_used_size)) {
    return NULL;
  }

  if (size == header->size) {
    return p;
  }

  FreeBlock* prior_block;
  FreeBlock* next_block;
  get_adjacent_blocks(&prior_block, &next_block, (U8*)p);
  // smaller size
  if (size < header->size) {
    realloc_smaller((U8*)p, size, prior_block, next_block);
    return p;
  }

  return realloc_bigger((U8*)p, size, prior_block, next_block);
}

void FreeListAllocator::al_free(void* p) {
  CHECK_LOG_RETURN(check_p_in_dev(p), "Invalid pointer to free");
  AllocHeader* header = get_allocation_header(p);
  SIP freed_size = header->size + ((U8*)p - header->start);
  m_used_size -= freed_size;
  FreeBlock* new_block = (FreeBlock*)header->start;
  new_block->size = freed_size;
  new_block->size = NULL;
  add_and_merge_free_block(new_block);
}

U8* FreeListAllocator::find_best_fit_free_block(FreeBlock** o_fit_block, FreeBlock** o_prior_block, SIP size, SIP alignment) {
  U8* p = NULL;
  FreeBlock* curr_block = m_first_block;
  FreeBlock* curr_prior_block = NULL;
  *o_fit_block = NULL;
  *o_prior_block = NULL;
  while (curr_block) {
    U8* curr_p = (U8*)curr_block + sizeof(AllocHeader);
    // Align the returned pointer if neccessary.
    curr_p = align_forward(curr_p, alignment);
    if (curr_block->size >= (curr_p - (U8*)curr_block) + size) {
      if (!*(o_fit_block) || (curr_block->size < (*o_fit_block)->size)) {
        *o_fit_block = curr_block;
        *o_prior_block = curr_prior_block;
        p = curr_p;
      }
    }
    curr_prior_block = curr_block;
    curr_block = curr_block->next;
  }
  if (!(*o_fit_block)) {
    return NULL;
  }
  return p;
}

void FreeListAllocator::get_adjacent_blocks(FreeBlock** prior_block, FreeBlock** next_block, U8* p) {
  *prior_block = NULL;
  *next_block = m_first_block;
  while (*next_block && p > (U8*)(*next_block)) {
    *prior_block = *next_block;
    *next_block = (*next_block)->next;
  }
}

void FreeListAllocator::link_separated_blocks(FreeBlock* prior_block, FreeBlock* block) {
  if (prior_block) {
    prior_block->next = block;
  } else {
    m_first_block = block;
  }
}

void FreeListAllocator::link_and_merge_free_blocks(FreeBlock** first, FreeBlock** second) {
  if (!*first) {
    m_first_block = *second;
    return;
  }
  // Merge two blocks if they are contiguous.
  if ((U8*)(*first) + (*first)->size == (U8*)(*second)) {
    (*first)->size += (*second)->size;
    (*first)->next = (*second)->next;
    *second = *first;
  } else {
    (*first)->next = *second;
  }
}

void FreeListAllocator::add_and_merge_free_block(FreeBlock* block) {
  FreeBlock* prior_block = NULL;
  FreeBlock* next_block = m_first_block;
  get_adjacent_blocks(&prior_block, &next_block, (U8*)block);
  if (prior_block) {
    link_and_merge_free_blocks( &prior_block, &block);
  } else {
    m_first_block = block;
  }

  if (next_block) {
    link_and_merge_free_blocks(&block, &next_block);
  }
}

bool FreeListAllocator::shrink_free_block(FreeBlock* block, FreeBlock* prior_block, SIP size) {
  SIP block_size_after = block->size - size;
  if (!is_enough_for_allocation_header(block_size_after)) {
    if (prior_block) {
      prior_block->next = block->next;
    } else {
      m_first_block = block->next;
    }
    m_used_size += block->size;
    return false;
  }
  FreeBlock* shrunk_block = (FreeBlock*)((U8*)block + size);
  *shrunk_block = (FreeBlock){.size = block_size_after, .next = block->next};
  link_separated_blocks(prior_block, shrunk_block);
  m_used_size += size;
  return true;
}

void FreeListAllocator::realloc_smaller(U8* p, SIP size, FreeBlock* prior_block, FreeBlock* next_block) {
  AllocHeader* header = get_allocation_header(p);
  SIP size_after_shrunk = header->size - size;
  // If there is a FreeBlock right after the allocation, we shift the
  // FreeBlock to the end of the new allocation.
  if (is_allocaiton_adjacent_to_free_block(header, next_block)) {
    header->size = size;
    FreeBlock* shifted_block = (FreeBlock*)(p + size);
    shifted_block->size = next_block->size + size_after_shrunk;
    shifted_block->next = next_block->next;
    link_and_merge_free_blocks(&prior_block, &shifted_block);
    m_used_size -= size_after_shrunk;
    return;
  }
  // There is not enough space for a new FreeBlock, nothing changes.
  if (!is_enough_for_allocation_header(size_after_shrunk)) {
    return;
  }
  // Else, we create a new FreeBlock.
  header->size = size;
  FreeBlock* new_block = (FreeBlock*)(p + size);
  new_block->size = size_after_shrunk;
  new_block->next = next_block;
  link_separated_blocks(prior_block, new_block);
  m_used_size -= size_after_shrunk;
}

U8* FreeListAllocator::realloc_bigger(U8* p, SIP size, FreeBlock* prior_block, FreeBlock* next_block) {
  AllocHeader* header = get_allocation_header(p);
  // Check if |next_block| is adjacent to |p| so we may extend |p|.
  if (is_allocaiton_adjacent_to_free_block(header, next_block)) {
    if (header->size + next_block->size >= size) {
      SIP size_after_extended = header->size + next_block->size - size;
      if (is_enough_for_allocation_header(size_after_extended)) {
        FreeBlock* new_block = (FreeBlock*)(p + size);
        new_block->size = size_after_extended;
        new_block->next = next_block->next;
        link_and_merge_free_blocks(&prior_block, &new_block);
        m_used_size += size - header->size;
        header->size = size;
      } else {
        m_used_size += next_block->size;
        header->size += next_block->size;
        link_separated_blocks(prior_block, next_block->next);
      }
      return p;
    }
  }
  // Else, find other FreeBlock.
  // If sizeof(AllocHeader) is smaller than sizeof(FreeBlock), then
  // the |new_block| will overwrite the data of p
  AllocHeader backup_header = *header;
  FreeBlock backup_prior_block;
  FreeBlock backup_next_block;
  if (prior_block) {
    backup_prior_block = *prior_block;
  }
  if (next_block) {
    backup_next_block = *next_block;
  }
  FreeBlock* backup_m_first_block = m_first_block;
  FreeBlock* new_block = (FreeBlock*)backup_header.start;
  new_block->size = (SIP)(header->size + (p - header->start));
  new_block->next = NULL;
  add_and_merge_free_block(new_block);
  FreeBlock* fit_block;
  FreeBlock* prior_fit_block;
  U8* returned_pointer = find_best_fit_free_block(&fit_block, &prior_fit_block, size, backup_header.alignment);
  if (returned_pointer) {
    m_used_size -= backup_header.size + (p - backup_header.start);
    memmove(returned_pointer, p, size);
    SIP padding_and_header = returned_pointer - (U8*)fit_block;
    bool rv = shrink_free_block(fit_block, prior_fit_block, padding_and_header + size);
    header = get_allocation_header(returned_pointer);
    header->start = (U8*)fit_block;
    header->size = rv ? size : fit_block->size - padding_and_header;
    header->alignment = backup_header.alignment;
#if IS_DEV()
    header->p = returned_pointer;
#endif
    return returned_pointer;
  }
  // Restores |header|
  if (prior_block) {
    *prior_block = backup_prior_block;
  }
  if (next_block) {
    *next_block = backup_next_block;
  }
  m_first_block = backup_m_first_block;
  *header = backup_header;
  LOGD("Free list allocator \"%s\" doesn't have enough space to alloc %d bytes", m_name, size);
  return NULL;
}

