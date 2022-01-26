//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/free_list_allocator.h"

#include "core/allocator_internal.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

// Free_block_t_ and allocations use a shared memory region.
struct Free_block_t_ {
  /// |size| includes this struct.
  Sip size;
  Free_block_t_* next;
};

static_assert(
    sizeof(Alloc_header_t_) >= sizeof(Free_block_t_),
    "We assume that sizeof(Alloc_header_t_) >= sizeof(Free_block_t_) so when "
    "freeating an allocation, we always have space for a Free_block_t_");

// Returns true if |size| is bigger than sizeof(Alloc_header_t_) which we
// assume that it is bigger than sizeof(Free_block_t_).
static bool is_enough_for_allocation_header_(Sip size) {
  return size > sizeof(Alloc_header_t_);
}

static bool is_allocaiton_adjacent_to_free_block_(Alloc_header_t_* header, Free_block_t_* block) {
  return block && (U8*)header + sizeof(Alloc_header_t_) + header->size == (U8*)block;
}

bool Free_list_allocator_t::init() {
  m_used_size = 0;
  m_start = (U8*)malloc(m_total_size);
  M_check_log_return_val(m_start, false, "Can't init allocator \"%s\": Out of memory", m_name);
  m_first_block = (Free_block_t_*)m_start;
  m_first_block->size = m_total_size;
  m_first_block->next = NULL;
  return true;
}

void Free_list_allocator_t::destroy() {
  if (m_start) {
    ::free(m_start);
  }
}

void* Free_list_allocator_t::aligned_alloc(Sip size, Sip alignment) {
  M_check_log_return_val(check_aligned_alloc_(size, alignment), NULL, "Alignment is not power of 2");
  Free_block_t_* fit_block;
  Free_block_t_* prior_block;
  U8* p = find_best_fit_free_block_(&fit_block, &prior_block, size, alignment);
  M_check_log_return_val(p, NULL, "Free list allocator \"%s\" doesn't have enough space to alloc %d bytes", m_name, size);
  Sip padding_and_header = p - (U8*)fit_block;
  bool rv = shrink_free_block_(fit_block, prior_block, padding_and_header + size);
  Alloc_header_t_* hdr = get_allocation_header_(p);
  hdr->start = (U8*)fit_block;
  hdr->size = rv ? size : fit_block->size - padding_and_header;
  hdr->alignment = alignment;
#if M_is_dev()
  hdr->p = p;
#endif
  return p;
}

void* Free_list_allocator_t::realloc(void* p, Sip size) {
  M_check_log_return_val(check_p_in_dev_(p) && size, NULL, "Invalid pointer to realloc");

  Alloc_header_t_* header = get_allocation_header_(p);
  // Remaining free space is surely not enough.
  if (size > header->size + (m_total_size - m_used_size)) {
    return NULL;
  }

  if (size == header->size) {
    return p;
  }

  Free_block_t_* prior_block;
  Free_block_t_* next_block;
  get_adjacent_blocks_(&prior_block, &next_block, (U8*)p);
  // smaller size
  if (size < header->size) {
    realloc_smaller_((U8*)p, size, prior_block, next_block);
    return p;
  }

  return realloc_bigger_((U8*)p, size, prior_block, next_block);
}

void Free_list_allocator_t::free(void* p) {
  M_check_log_return(check_p_in_dev_(p), "Invalid pointer to free");
  Alloc_header_t_* header = get_allocation_header_(p);
  Sip freed_size = header->size + ((U8*)p - header->start);
  m_used_size -= freed_size;
  Free_block_t_* new_block = (Free_block_t_*)header->start;
  new_block->size = freed_size;
  new_block->size = NULL;
  add_and_merge_free_block_(new_block);
}

U8* Free_list_allocator_t::find_best_fit_free_block_(Free_block_t_** o_fit_block, Free_block_t_** o_prior_block, Sip size, Sip alignment) {
  U8* p = NULL;
  Free_block_t_* curr_block = m_first_block;
  Free_block_t_* curr_prior_block = NULL;
  *o_fit_block = NULL;
  *o_prior_block = NULL;
  while (curr_block) {
    U8* curr_p = (U8*)curr_block + sizeof(Alloc_header_t_);
    // Align the returned pointer if neccessary.
    curr_p = align_forward_(curr_p, alignment);
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

void Free_list_allocator_t::get_adjacent_blocks_(Free_block_t_** prior_block, Free_block_t_** next_block, U8* p) {
  *prior_block = NULL;
  *next_block = m_first_block;
  while (*next_block && p > (U8*)(*next_block)) {
    *prior_block = *next_block;
    *next_block = (*next_block)->next;
  }
}

void Free_list_allocator_t::link_separated_blocks_(Free_block_t_* prior_block, Free_block_t_* block) {
  if (prior_block) {
    prior_block->next = block;
  } else {
    m_first_block = block;
  }
}

void Free_list_allocator_t::link_and_merge_free_blocks_(Free_block_t_** first, Free_block_t_** second) {
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

void Free_list_allocator_t::add_and_merge_free_block_(Free_block_t_* block) {
  Free_block_t_* prior_block = NULL;
  Free_block_t_* next_block = m_first_block;
  get_adjacent_blocks_(&prior_block, &next_block, (U8*)block);
  if (prior_block) {
    link_and_merge_free_blocks_( &prior_block, &block);
  } else {
    m_first_block = block;
  }

  if (next_block) {
    link_and_merge_free_blocks_(&block, &next_block);
  }
}

bool Free_list_allocator_t::shrink_free_block_(Free_block_t_* block, Free_block_t_* prior_block, Sip size) {
  Sip block_size_after = block->size - size;
  if (!is_enough_for_allocation_header_(block_size_after)) {
    if (prior_block) {
      prior_block->next = block->next;
    } else {
      m_first_block = block->next;
    }
    m_used_size += block->size;
    return false;
  }
  Free_block_t_* shrunk_block = (Free_block_t_*)((U8*)block + size);
  *shrunk_block = (Free_block_t_){.size = block_size_after, .next = block->next};
  link_separated_blocks_(prior_block, shrunk_block);
  m_used_size += size;
  return true;
}

void Free_list_allocator_t::realloc_smaller_(U8* p, Sip size, Free_block_t_* prior_block, Free_block_t_* next_block) {
  Alloc_header_t_* header = get_allocation_header_(p);
  Sip size_after_shrunk = header->size - size;
  // If there is a Free_block_t_ right after the allocation, we shift the
  // Free_block_t_ to the end of the new allocation.
  if (is_allocaiton_adjacent_to_free_block_(header, next_block)) {
    header->size = size;
    Free_block_t_* shifted_block = (Free_block_t_*)(p + size);
    shifted_block->size = next_block->size + size_after_shrunk;
    shifted_block->next = next_block->next;
    link_and_merge_free_blocks_(&prior_block, &shifted_block);
    m_used_size -= size_after_shrunk;
    return;
  }
  // There is not enough space for a new Free_block_t_, nothing changes.
  if (!is_enough_for_allocation_header_(size_after_shrunk)) {
    return;
  }
  // Else, we create a new Free_block_t_.
  header->size = size;
  Free_block_t_* new_block = (Free_block_t_*)(p + size);
  new_block->size = size_after_shrunk;
  new_block->next = next_block;
  link_separated_blocks_(prior_block, new_block);
  m_used_size -= size_after_shrunk;
}

U8* Free_list_allocator_t::realloc_bigger_(U8* p, Sip size, Free_block_t_* prior_block, Free_block_t_* next_block) {
  Alloc_header_t_* header = get_allocation_header_(p);
  // Check if |next_block| is adjacent to |p| so we may extend |p|.
  if (is_allocaiton_adjacent_to_free_block_(header, next_block)) {
    if (header->size + next_block->size >= size) {
      Sip size_after_extended = header->size + next_block->size - size;
      if (is_enough_for_allocation_header_(size_after_extended)) {
        Free_block_t_* new_block = (Free_block_t_*)(p + size);
        new_block->size = size_after_extended;
        new_block->next = next_block->next;
        link_and_merge_free_blocks_(&prior_block, &new_block);
        m_used_size += size - header->size;
        header->size = size;
      } else {
        m_used_size += next_block->size;
        header->size += next_block->size;
        link_separated_blocks_(prior_block, next_block->next);
      }
      return p;
    }
  }
  // Else, find other Free_block_t_.
  // If sizeof(Alloc_header_t_) is smaller than sizeof(Free_block_t_), then
  // the |new_block| will overwrite the data of p
  Alloc_header_t_ backup_header = *header;
  Free_block_t_ backup_prior_block;
  Free_block_t_ backup_next_block;
  if (prior_block) {
    backup_prior_block = *prior_block;
  }
  if (next_block) {
    backup_next_block = *next_block;
  }
  Free_block_t_* backup_m_first_block = m_first_block;
  Free_block_t_* new_block = (Free_block_t_*)backup_header.start;
  new_block->size = (Sip)(header->size + (p - header->start));
  new_block->next = NULL;
  add_and_merge_free_block_(new_block);
  Free_block_t_* fit_block;
  Free_block_t_* prior_fit_block;
  U8* returned_pointer = find_best_fit_free_block_(&fit_block, &prior_fit_block, size, backup_header.alignment);
  if (returned_pointer) {
    m_used_size -= backup_header.size + (p - backup_header.start);
    memmove(returned_pointer, p, size);
    Sip padding_and_header = returned_pointer - (U8*)fit_block;
    bool rv = shrink_free_block_(fit_block, prior_fit_block, padding_and_header + size);
    header = get_allocation_header_(returned_pointer);
    header->start = (U8*)fit_block;
    header->size = rv ? size : fit_block->size - padding_and_header;
    header->alignment = backup_header.alignment;
#if M_is_dev()
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
  M_logd("Free list allocator \"%s\" doesn't have enough space to alloc %d bytes", m_name, size);
  return NULL;
}

