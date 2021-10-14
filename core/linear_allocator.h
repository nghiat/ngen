//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/allocator.h"
#include "core/ng_types.h"

struct LAPage;

// The idea of this allocator is simple.
// It contains a linked list of pages.
// Whenever you allocate from this allocator, it tries to advance the current |m_top| pointer.
// If the current page doesn't have enough space, it will allocate another page that is >= |sc_default_page_size| depending on the size of the allocation.
// The first page is stack memory which will probably fit most of its usage.
// Realloc and free only works with the last allocation to keep it simple.
template <SZ INITIAL_SIZE = 4096>
class LinearAllocator : public ngAllocator {
public:
  LinearAllocator(const char* name) : ngAllocator(name, INITIAL_SIZE) {}
  bool la_init();
  void al_destroy() override;
  void* al_aligned_alloc(SIP size, SIP alignment) override;
  void* al_realloc(void* p, SIP size) override;
  void al_free(void* p) override;

private:
  SIP get_current_page_remaning_size();

  static const SIP sc_default_page_size = 1024 * 1024;
  U8 m_stack_page[INITIAL_SIZE];
  LAPage* m_current_page;
  U8* m_top;
};
