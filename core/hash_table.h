//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/hash.h"
#include "core/ng_types.h"

#include <string.h>

#include <type_traits>

template <typename T>
class Equal_t {
public:
  bool operator()(const T& lhs, const T& rhs) {
    return lhs == rhs;
  }
};

template <>
class Equal_t<const char*> {
public:
  bool operator()(const char* const& lhs, const char* const& rhs) {
    return strcmp(lhs, rhs) == 0;
  }
};

// This hash table uses linear probing.
// There is another implementation in hash_table2.h which uses separate chaining.
// But in a preliminary benchmark, Hash_table2 showed is significantly slower, even when it uses array for chaining instead of linked list.
template <typename T_key, typename T_value, typename T_data, typename T_hash, typename T_equal>
class Hash_table_t_ {
public:
  class Iterator_t_ {
  public:
    void operator++();
    T_data& operator*();
    bool operator!=(const Iterator_t_& rhs);

    const Hash_table_t_<T_key, T_value, T_data, T_hash, T_equal>* m_ht;
    Sz m_idx;
  };

  bool init(Allocator_t* allocator);
  void destroy();
  T_value& operator[](const T_key& key);
  T_value* find(const T_key& key) const;
  Sip len() const;
  void reserve(int key_count);

// iterator (for each)
  Iterator_t_ begin() const;
  Iterator_t_ end() const;

// private:
  enum E_slot_state : U8 {
    e_slot_state_free,
    e_slot_state_alive,
    e_slot_state_moving, // only when rehashing
    e_slot_state_dead,
  };
  Sz get_bucket_index(const T_key& key) const;
  void rehash(int bucket_count);
  T_value& insert_without_checking(int bucket_idx, const T_key& key);

  Allocator_t* m_allocator = nullptr;
  // |m_data| contains 2 array: data array and state array.
  // The state array comes after the data array so we can resize both of them using only one realloc call.
  // Which works well with the Linear_allocator_t because it can only realloc to a bigger size when the pointer is at the top.
  Dynamic_array_t<U8> m_data;
  T_data* m_data_p = nullptr;
  E_slot_state* m_states_p = nullptr;
  Sz m_bucket_count = 0;
  Sz m_count = 0;
  S32 m_initial_bucket_count = 4;
  // Used to calculate load factor
  F32 m_load_factor = 0.5f;
};

template <typename T_key, typename T_value>
struct Pair_t_ {
  T_key key;
  T_value value;
};

template <typename T>
union FakePair_ {
  T key;
  T value;
};

template <typename T_key, typename T_value, typename T_hash = Hash_t<std::remove_const_t<T_key>>, typename T_equal = Equal_t<T_key>>
using Hash_map = Hash_table_t_<T_key, T_value, Pair_t_<T_key, T_value>, T_hash, T_equal>;

template <typename T_key, typename T_hash = Hash_t<std::remove_const_t<T_key>>, typename T_equal = Equal_t<T_key>>
using Hash_set = Hash_table_t_<T_key, T_key, Pair_t_<T_key, T_key>, T_hash, T_equal>;
