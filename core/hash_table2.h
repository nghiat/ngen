//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/hash.h"
#include "core/ng_types.h"

// See comment in hash_table.h
template <typename T_key, typename T_value, typename T_data, typename T_hash>
class Hash_table2_ {
public:
  struct Bucket_ {
    // We don't use Dynamic_array because we want keep the size of this struct to minimal
    T_data* chain = nullptr;
    Sip len = 0;
  };

  bool init(Allocator* allocator);
  void destroy();
  T_value& operator[](const T_key& key);
  T_value* find(const T_key& key);
  Sip len() const;
  void reserve(int key_count);

// private:

  Sz get_bucket_index(const T_key& key);
  void rehash(int bucket_count);
  T_value& insert_without_checking(int bucket_idx, const T_key& key);

  Allocator* m_allocator = nullptr;
  Dynamic_array<Bucket_> m_buckets;
  Sz m_count = 0;
  S32 m_initial_bucket_count = 4;
  // Used to calculate load factor
  S32 m_average_entries_per_bucket = 1;
  F32 m_load_factor = 0.5f;
};

template <typename T_key, typename T_value, typename T_hash = Ng_hash<T_key>>
using Hash_map2 = Hash_table2_<T_key, T_value, Pair_<T_key, T_value>, T_hash>;

template <typename T_key, typename T_hash = Ng_hash<T_key>>
using Hash_set2 = Hash_table2_<T_key, T_key, Pair_<T_key, T_key>, T_hash>;
