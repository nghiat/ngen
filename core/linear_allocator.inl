//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 201             //
//----------------------------------------------------------------------------//

#include "core/linear_allocator.h"

#include "core/allocator_internal.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

struct LAPage {
  SIP size;
  LAPage* prev;
};

template <SZ INITIAL_SIZE>
bool LinearAllocator<INITIAL_SIZE>::la_init() {
  m_used_size += sizeof(LAPage);
  m_current_page = (LAPage*)&(m_stack_page[0]);
  m_current_page->size = INITIAL_SIZE;
  m_current_page->prev = NULL;
  m_top = (U8*)(m_current_page + 1);
  return true;
}

template <SZ INITIAL_SIZE>
void LinearAllocator<INITIAL_SIZE>::al_destroy() {
  LAPage* page = m_current_page;
  while (page != (LAPage*)&(m_stack_page[0])) {
    LAPage* prev = page->prev;
    ::free(page);
    page = prev;
  }
}

template <SZ INITIAL_SIZE>
void* LinearAllocator<INITIAL_SIZE>::al_aligned_alloc(SIP size, SIP alignment) {
  CHECK_LOG_RETURN_VAL(check_aligned_alloc(size, alignment), NULL, "Alignment is not power of 2");

  U8* p = m_top + sizeof(AllocHeader);
  p = align_forward(p, alignment);
  SIP real_size = (p - m_top) + size;
  if (get_current_page_remaning_size() < real_size) {
    // Create a new page.
    SIP new_page_size = sizeof(LAPage) + sizeof(AllocHeader) + size + alignment;
    if (new_page_size < sc_default_page_size)
      new_page_size = sc_default_page_size;
    LAPage* new_page = (LAPage*)malloc(new_page_size);
    CHECK_LOG_RETURN_VAL(new_page, NULL, "Out of memory for new page for linear allocator \"%s\"", m_name);
    m_total_size += new_page_size;
    m_used_size += get_current_page_remaning_size() + sizeof(LAPage);
    new_page->size = new_page_size;
    new_page->prev = m_current_page;
    m_current_page = new_page;
    m_top = (U8*)(m_current_page + 1);
    p = align_forward(m_top + sizeof(AllocHeader), alignment);
    real_size = (p - m_top) + size;
  }
  AllocHeader* hdr = get_allocation_header(p);
  hdr->start = m_top;
  hdr->size = size;
  hdr->alignment = alignment;
#if IS_DEV()
  hdr->p = p;
#endif
  m_top += real_size;
  m_used_size += real_size;
  return p;
}

template <SZ INITIAL_SIZE>
void* LinearAllocator<INITIAL_SIZE>::al_realloc(void* p, SIP size) {
  CHECK_LOG_RETURN_VAL(check_p_in_dev(p) && size, NULL, "Invalid pointer to realloc");

  AllocHeader* header = get_allocation_header(p);
  SIP old_size = header->size;
  // Not at top
  if ((U8*)p + header->size != m_top) {
    void* new_p = al_aligned_alloc(size, header->alignment);
    memcpy(new_p, p, header->size);
    return new_p;
  }
  // Remaining space is not enough.
  if (size > get_current_page_remaning_size()) {
    void* new_p = al_aligned_alloc(size, header->alignment);
    memcpy(new_p, p, header->size);
    return new_p;
  }
  if (size == old_size)
    return p;
  m_used_size = m_used_size + size - old_size;
  header->size = size;
  m_top = (U8*)p + size;
  return p;
}

template <SZ INITIAL_SIZE>
void LinearAllocator<INITIAL_SIZE>::al_free(void* p) {
  CHECK_LOG_RETURN(check_p_in_dev(p), "Invalid pointer to free");
  AllocHeader* header = get_allocation_header(p);
  if ((U8*)p + header->size != m_top) {
    return;
  }
  m_top = header->start;
  m_used_size -= header->size + ((U8*)p - header->start);
}

template <SZ INITIAL_SIZE>
SIP LinearAllocator<INITIAL_SIZE>::get_current_page_remaning_size() {
  SIP remaining_size = m_current_page->size - (m_top - (U8*)(m_current_page));
  CHECK(remaining_size >= 0);
  return remaining_size;
}

