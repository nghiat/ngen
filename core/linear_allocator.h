//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/allocator.h"
#include "core/types.h"

struct Linear_allocator_page_t_;

// The idea of this allocator is simple.
// It contains a linked list of pages.
// Whenever you allocate from this allocator, it tries to advance the current |m_top| pointer.
// If the current page doesn't have enough space, it will allocate another page that is >= |sc_default_page_size| depending on the size of the allocation.
// The first page is stack memory which will probably fit most of its usage.
// Realloc and free only works with the last allocation to keep it simple.
template <Sz T_initial_size = 4096>
class Linear_allocator_t : public Allocator_t {
public:
  Linear_allocator_t(const char* name);
  void destroy() override;
  void* aligned_alloc(Sip size, Sip alignment) override;
  void* realloc(void* p, Sip size) override;
  void free(void* p) override;

  static const Sip sc_default_page_size = 32 * 1024 * 1024;
  U8 m_stack_page[T_initial_size];
  Linear_allocator_page_t_* m_current_page;
  Linear_allocator_page_t_* m_first_page;
  U8* m_top;

private:
  Sip get_current_page_remaning_size_();
};

template <Sz T = 4096>
class Scope_allocator_t : public Allocator_t {
public:
  Scope_allocator_t(Linear_allocator_t<T>* allocator) : Allocator_t("scope_allocator", T), m_main_allocator(allocator), m_main_allocator_snapshot(*allocator) {}
  ~Scope_allocator_t();
  void destroy() override;
  void* aligned_alloc(Sip size, Sip alignment) override;
  void* realloc(void* p, Sip size) override;
  void free(void* p) override;

  Linear_allocator_t<T>* m_main_allocator = NULL;
  Linear_allocator_t<T> m_main_allocator_snapshot;
};
