//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

class Allocator_t;

template <typename T>
class Intrusive_list_t {
public:
  void init_head();
  void append(Intrusive_list_t<T>* child);

// iterator (for each)
  class Iterator_t_ {
  public:
    Intrusive_list_t<T>* m_node;
    bool operator!=(const Iterator_t_& other) const;
    Iterator_t_& operator++();
    T& operator*();
  };
  Iterator_t_ begin() const;
  Iterator_t_ end() const;

  Intrusive_list_t<T>* m_next = nullptr;
  Intrusive_list_t<T>* m_prev = nullptr;
};
