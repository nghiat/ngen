//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/ng_types.h"

// This hash table uses linear probing.
// There is another implementation in hash_table2.h which uses separate chaining.
// But in a preliminary benchmark, Hash_table2 showed is significantly slower, even when it uses array for chaining instead of linked list.
template <typename T_key, typename T_value, typename T_data>
class Hash_table_ {
public:
  bool init(Allocator* allocator);
  void destroy();
  T_value& operator[](const T_key& key);
  T_value* find(const T_key& key);
  Sip len() const;
  void reserve(int num_keys);

// private:
  enum E_slot_state : U8 {
    e_slot_state_free,
    e_slot_state_alive,
    e_slot_state_moving, // only when rehashing
    e_slot_state_dead,
  };
  Sz get_bucket_index(const T_key& key);
  void rehash(int old_num_buckets);
  T_value& insert_without_checking(int bucket_idx, const T_key& key);

  Allocator* m_allocator = nullptr;
  // |m_data| contains 2 array: data array and state array.
  // The state array comes after the data array so we can resize both of them using only one realloc call.
  // Which works well with the Linear_allocator because it can only realloc to a bigger size when the pointer is at the top.
  Dynamic_array<U8> m_data;
  T_data* m_data_p = nullptr;
  E_slot_state* m_states_p = nullptr;
  Sz m_num_buckets = 0;
  Sz m_count = 0;
  S32 m_initial_num_buckets = 4;
  // Used to calculate load factor
  F32 m_load_factor = 0.5f;
};
