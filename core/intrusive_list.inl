//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/intrusive_list.h"

#include "core/allocator.h"

template <typename T>
void Intrusive_list_t<T>::init_head() {
  m_next = this;
  m_prev = this;
}

template <typename T>
void Intrusive_list_t<T>::append(Intrusive_list_t<T>* child) {
  child->m_prev = m_prev;
  child->m_next = this;
  m_prev->m_next = child;
  m_prev = child;
}

template <typename T>
bool Intrusive_list_t<T>::Iterator_t_::operator!=(const Intrusive_list_t<T>::Iterator_t_& other) const {
  return m_node != other.m_node;
}

template <typename T>
typename Intrusive_list_t<T>::Iterator_t_& Intrusive_list_t<T>::Iterator_t_::operator++() {
  m_node = m_node->m_next;
  return *this;
}

template <typename T>
T& Intrusive_list_t<T>::Iterator_t_::operator*() {
  return *(T*)m_node;
}

template <typename T>
typename Intrusive_list_t<T>::Iterator_t_ Intrusive_list_t<T>::begin() const {
  return Iterator_t_{m_next};
}

template <typename T>
typename Intrusive_list_t<T>::Iterator_t_ Intrusive_list_t<T>::end()const {
  return Iterator_t_{m_prev->m_next};
}
