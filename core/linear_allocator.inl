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

struct LAPage_ {
  Sip size;
  LAPage_* prev;
};

template <Sz T_initial_size>
bool Linear_allocator<T_initial_size>::init() {
  m_used_size += sizeof(LAPage_);
  m_current_page = (LAPage_*)&(m_stack_page[0]);
  m_current_page->size = T_initial_size;
  m_current_page->prev = NULL;
  m_top = (U8*)(m_current_page + 1);
  return true;
}

template <Sz T_initial_size>
void Linear_allocator<T_initial_size>::destroy() {
  LAPage_* page = m_current_page;
  while (page != (LAPage_*)&(m_stack_page[0])) {
    LAPage_* prev = page->prev;
    ::free(page);
    page = prev;
  }
}

template <Sz T_initial_size>
void* Linear_allocator<T_initial_size>::aligned_alloc(Sip size, Sip alignment) {
  M_check_log_return_val(check_aligned_alloc_(size, alignment), NULL, "Alignment is not power of 2");

  U8* p = m_top + sizeof(Alloc_header_);
  p = align_forward_(p, alignment);
  Sip real_size = (p - m_top) + size;
  if (get_current_page_remaning_size_() < real_size) {
    // Create a new page.
    Sip new_page_size = sizeof(LAPage_) + sizeof(Alloc_header_) + size + alignment;
    if (new_page_size < sc_default_page_size)
      new_page_size = sc_default_page_size;
    LAPage_* new_page = (LAPage_*)malloc(new_page_size);
    M_check_log_return_val(new_page, NULL, "Out of memory for new page for linear allocator \"%s\"", m_name);
    m_total_size += new_page_size;
    m_used_size += get_current_page_remaning_size_() + sizeof(LAPage_);
    new_page->size = new_page_size;
    new_page->prev = m_current_page;
    m_current_page = new_page;
    m_top = (U8*)(m_current_page + 1);
    p = align_forward_(m_top + sizeof(Alloc_header_), alignment);
    real_size = (p - m_top) + size;
  }
  Alloc_header_* hdr = get_allocation_header_(p);
  hdr->start = m_top;
  hdr->size = size;
  hdr->alignment = alignment;
#if M_is_dev()
  hdr->p = p;
#endif
  m_top += real_size;
  m_used_size += real_size;
  return p;
}

template <Sz T_initial_size>
void* Linear_allocator<T_initial_size>::realloc(void* p, Sip size) {
  M_check_log_return_val(check_p_in_dev_(p) && size, NULL, "Invalid pointer to realloc");

  Alloc_header_* header = get_allocation_header_(p);
  Sip old_size = header->size;
  // Not at top
  if ((U8*)p + header->size != m_top) {
    // TODO test this after free top
    if (size < old_size) {
      return p;
    }
    void* new_p = aligned_alloc(size, header->alignment);
    memcpy(new_p, p, header->size);
    return new_p;
  }
  // Remaining space is not enough.
  if (size > get_current_page_remaning_size_()) {
    void* new_p = aligned_alloc(size, header->alignment);
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

template <Sz T_initial_size>
void Linear_allocator<T_initial_size>::free(void* p) {
  M_check_log_return(check_p_in_dev_(p), "Invalid pointer to free");
  Alloc_header_* header = get_allocation_header_(p);
  if ((U8*)p + header->size != m_top) {
    return;
  }
  m_top = header->start;
  m_used_size -= header->size + ((U8*)p - header->start);
}

template <Sz T_initial_size>
Sip Linear_allocator<T_initial_size>::get_current_page_remaning_size_() {
  Sip remaining_size = m_current_page->size - (m_top - (U8*)(m_current_page));
  M_check(remaining_size >= 0);
  return remaining_size;
}

