//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/hash_table.h"

#include "core/dynamic_array.inl"
#include "core/hash.h"
#include "core/utils.h"

#define M_hash_table_t_ template <typename T_key, typename T_value, typename T_data>
#define M_hash_table_c_ Hash_table_<T_key, T_value, T_data>

M_hash_table_t_
bool M_hash_table_c_::init(Allocator* allocator) {
  m_allocator = allocator;
  bool rv = m_data.init(allocator);
  return rv;
}

M_hash_table_t_
void M_hash_table_c_::destroy() {
  // TODO: call destroy
  m_data.destroy();
}

M_hash_table_t_
T_value& M_hash_table_c_::operator[](const T_key& key) {
  T_value* found_value = find(key);
  if (found_value) {
    return *found_value;
  }

  if (m_num_buckets == 0 || m_count + 1 > m_load_factor * m_num_buckets) {
    int old_num_buckets = m_num_buckets;
    int new_num_buckets = old_num_buckets * 3 / 2;
    if (old_num_buckets == 0) {
      new_num_buckets = m_initial_num_buckets;
    }
    reserve(new_num_buckets);
    if (old_num_buckets) {
      rehash(old_num_buckets);
    }
  }
  ++m_count;
  Sip starting_bucket_idx = get_bucket_index(key);
  for (int j = 0; j < m_num_buckets; ++j) {
    int bucket_idx = (starting_bucket_idx + j) % m_num_buckets;
    E_slot_state& state = m_states_p[bucket_idx];
    if (state != e_slot_state_alive) {
      m_data_p[bucket_idx].key = key;
      state = e_slot_state_alive;
      found_value = &m_data_p[bucket_idx].value;
      break;
    }
  }
  return *found_value;
}

M_hash_table_t_
T_value* M_hash_table_c_::find(const T_key& key) {
  if (!m_count) {
    return nullptr;
  }
  Sip first_bucket_idx = get_bucket_index(key);
  T_value* rv = nullptr;
  for (int i = 0; i < m_num_buckets; ++i) {
    Sip bucket_idx = (first_bucket_idx + i) % m_num_buckets;
    if (m_states_p[bucket_idx] == e_slot_state_free) {
      break;
    }
    if (m_states_p[bucket_idx] == e_slot_state_dead) {
      continue;
    }
    // e_slot_alive
    T_data& data = m_data_p[bucket_idx];
    if (data.key == key) {
      rv = &data.value;
      break;
    }
  }
  return rv;
}

M_hash_table_t_
void M_hash_table_c_::reserve(int num_keys) {
  int old_num_buckets = m_num_buckets;
  int new_num_buckets = num_keys / m_load_factor + 1;
  if (old_num_buckets >= new_num_buckets) {
    return;
  }
  m_num_buckets = new_num_buckets;
  m_data.resize(new_num_buckets * (sizeof(T_data) + sizeof(E_slot_state)));
  m_data_p = (T_data*)m_data.m_p;
  E_slot_state* new_states_p = (E_slot_state*)(m_data.m_p + new_num_buckets * sizeof(T_data));
  static_assert(e_slot_state_free == 0, "memset needs the enum value to be 0");
  memset(new_states_p, 0, new_num_buckets * sizeof(E_slot_state));
  if (old_num_buckets) {
    memmove(new_states_p, m_states_p, old_num_buckets);
    m_states_p = new_states_p;
    rehash(old_num_buckets);
  } else {
    m_states_p = new_states_p;
  }
}

M_hash_table_t_
Sz M_hash_table_c_::get_bucket_index(const T_key& key) {
  return ng_hash(key) % m_num_buckets;
}

M_hash_table_t_
void M_hash_table_c_::rehash(int old_num_buckets) {
  // Updating states
  for (int i = 0; i < old_num_buckets; ++i) {
    E_slot_state& state = m_states_p[i];
    if (state == e_slot_state_alive) {
      state = e_slot_state_moving;
    } else if (state == e_slot_state_dead) {
      state = e_slot_state_free;
    }
  }

  // Actually moving
  for (int i = 0; i < old_num_buckets; ++i) {
    E_slot_state& old_state = m_states_p[i];
    if (old_state != e_slot_state_moving) {
      continue;
    }
    T_data& old_data = m_data_p[i];
    int starting_bucket_idx = get_bucket_index(old_data.key);
    for (int j = 0; j < m_num_buckets; ++j) {
      int new_bucket_idx = (starting_bucket_idx + j) % m_num_buckets;
      if (new_bucket_idx == i) {
        old_state = e_slot_state_alive;
        break;
      }
      E_slot_state& new_state = m_states_p[new_bucket_idx];
      T_data& new_data = m_data_p[new_bucket_idx];
      if (new_state == e_slot_state_free) {
        new_data = old_data;
        old_state = e_slot_state_free;
        new_state = e_slot_state_alive;
        break;
      }
      if (new_state == e_slot_state_moving) {
        swap(&new_data, &old_data);
        new_state = e_slot_state_alive;
        // old_state is the same
        --i;
        break;
      }
    }
  }
}
