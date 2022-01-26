//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once
#include "core/hash_table2.h"

#include "core/dynamic_array.inl"

#define M_hash_table2_t_ template <typename T_key, typename T_value, typename T_data, typename T_hash>
#define M_hash_table2_c_ Hash_table2_t_<T_key, T_value, T_data, T_hash>

M_hash_table2_t_
bool M_hash_table2_c_::init(Allocator_t* allocator) {
  m_allocator = allocator;
  bool rv = m_buckets.init(allocator);
  return rv;
}

M_hash_table2_t_
void M_hash_table2_c_::destroy() {
  // TODO: call destroy
  for (int i = 0; i < m_buckets.len(); ++i) {
    if (m_buckets[i].len > 1) {
      m_allocator->free(m_buckets[i].chain);
    }
  }
  m_buckets.destroy();
}

M_hash_table2_t_
T_value& M_hash_table2_c_::operator[](const T_key& key) {
  T_value* found_value = find(key);
  if (found_value) {
    return *found_value;
  }

  if (m_buckets.len() == 0 || m_count + 1 > m_load_factor * m_buckets.len() * m_average_entries_per_bucket) {
    int bucket_count = m_buckets.len();
    int new_bucket_count = bucket_count * 3 / 2;
    if (bucket_count == 0) {
      new_bucket_count = m_initial_bucket_count;
    }
    m_buckets.resize(new_bucket_count);
    for (int i = bucket_count; i < new_bucket_count; ++i) {
      m_buckets[i] = Bucket_t_();
    }
    if (bucket_count) {
      rehash(bucket_count);
    }
  }
  ++m_count;
  Sip bucket_idx = get_bucket_index(key);
  return insert_without_checking(bucket_idx, key);
}

M_hash_table2_t_
T_value* M_hash_table2_c_::find(const T_key& key) {
  if (m_buckets.len()) {
    Sip bucket_idx = get_bucket_index(key);
    Bucket_t_& bucket = m_buckets[bucket_idx];
    if (bucket.len) {
      for (int i = 0; i < bucket.len; ++i) {
        if (bucket.chain[i].key == key) {
          return &bucket.chain[i].value;
        }
      }
    }
  }
  return nullptr;
}

M_hash_table2_t_
void M_hash_table2_c_::reserve(int key_count) {
  int bucket_count = m_buckets.len();
  int new_bucket_count = key_count / m_load_factor + 1;
  if (bucket_count >= new_bucket_count) {
    return;
  }
  m_buckets.resize(new_bucket_count);
  for (int i = bucket_count; i < new_bucket_count; ++i) {
    m_buckets[i] = Bucket_t_();
  }
  if (bucket_count) {
    rehash(bucket_count);
  }
}

M_hash_table2_t_
Sz M_hash_table2_c_::get_bucket_index(const T_key& key) {
  return T_hash()(key) % m_buckets.len();
}

M_hash_table2_t_
void M_hash_table2_c_::rehash(int bucket_count) {
  for (int i = 0; i < bucket_count; ++i) {
    Bucket_t_& bucket = m_buckets[i];
    if (bucket.len == 0) {
      continue;
    }
    for (int j = 0; j < bucket.len; ++j) {
      T_data& data = bucket.chain[j];
      int new_bucket_idx = get_bucket_index(data.key);
      if (new_bucket_idx == i) {
        continue;
      }
      insert_without_checking(new_bucket_idx, data.key) = data.value;
      // if |data| is not the last element in |chain|, then we swap |data| with the last element and decrease the |len| by 1
      if (j < bucket.len - 1) {
        data = bucket.chain[bucket.len - 1];
        --bucket.len;
        --j;
      }
    }
  }
}

M_hash_table2_t_
T_value& M_hash_table2_c_::insert_without_checking(int bucket_idx, const T_key& key) {
  Bucket_t_& bucket = m_buckets[bucket_idx];
  ++bucket.len;
  if (bucket.chain == nullptr) {
    bucket.chain = (T_data*)m_allocator->alloc(sizeof(T_data));
    bucket.chain[0].key = key;
  } else {
    bucket.chain = (T_data*)m_allocator->realloc(bucket.chain, bucket.len * sizeof(T_data));
    bucket.chain[bucket.len - 1].key = key;
  }
  return bucket.chain[bucket.len - 1].value;
}
