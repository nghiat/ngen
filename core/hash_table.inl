//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/hash_table.h"

#include "core/dynamic_array.inl"
#include "core/utils.h"

#define M_hash_table_t_ template <typename T_key, typename T_value, typename T_data, typename T_hash, typename T_equal>
#define M_hash_table_c_ Hash_table_t_<T_key, T_value, T_data, T_hash, T_equal>

M_hash_table_t_
void M_hash_table_c_::Iterator_t_::operator++() {
  while (++m_idx < m_ht->m_bucket_count && m_ht->m_states_p[m_idx] != e_slot_state_alive) {}
}

M_hash_table_t_
T_data& M_hash_table_c_::Iterator_t_::operator*() {
  return m_ht->m_data_p[m_idx];
}

M_hash_table_t_
bool M_hash_table_c_::Iterator_t_::operator!=(const M_hash_table_c_::Iterator_t_& rhs) {
  return m_ht != rhs.m_ht || m_idx != rhs.m_idx;
}

M_hash_table_t_
M_hash_table_c_::Hash_table_t_(Allocator_t* allocator) : m_data(allocator) {}

M_hash_table_t_
void M_hash_table_c_::destroy() {
  m_data.destroy();
}

M_hash_table_t_
T_value& M_hash_table_c_::operator[](const T_key& key) {
  T_value* found_value = find(key);
  if (found_value) {
    return *found_value;
  }

  if (m_bucket_count == 0 || m_count + 1 > m_load_factor * m_bucket_count) {
    int bucket_count = m_bucket_count;
    int new_bucket_count = bucket_count * 3 / 2;
    if (bucket_count == 0) {
      new_bucket_count = m_initial_bucket_count;
    }
    reserve(new_bucket_count);
    if (bucket_count) {
      rehash(bucket_count);
    }
  }
  ++m_count;
  Sip starting_bucket_idx = get_bucket_index(key);
  for (int j = 0; j < m_bucket_count; ++j) {
    int bucket_idx = (starting_bucket_idx + j) % m_bucket_count;
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
T_value* M_hash_table_c_::find(const T_key& key) const {
  if (!m_count) {
    return nullptr;
  }
  Sip first_bucket_idx = get_bucket_index(key);
  T_value* rv = nullptr;
  for (int i = 0; i < m_bucket_count; ++i) {
    Sip bucket_idx = (first_bucket_idx + i) % m_bucket_count;
    if (m_states_p[bucket_idx] == e_slot_state_free) {
      break;
    }
    if (m_states_p[bucket_idx] == e_slot_state_dead) {
      continue;
    }
    // e_slot_alive
    T_data& data = m_data_p[bucket_idx];
    if (T_equal()(data.key, key)) {
      rv = &data.value;
      break;
    }
  }
  return rv;
}

M_hash_table_t_
void M_hash_table_c_::reserve(int key_count) {
  int bucket_count = m_bucket_count;
  int new_bucket_count = key_count / m_load_factor + 1;
  if (bucket_count >= new_bucket_count) {
    return;
  }
  m_bucket_count = new_bucket_count;
  m_data.resize(new_bucket_count * (sizeof(T_data) + sizeof(E_slot_state)));
  m_data_p = (T_data*)m_data.m_p;
  E_slot_state* new_states_p = (E_slot_state*)(m_data.m_p + new_bucket_count * sizeof(T_data));
  static_assert(e_slot_state_free == 0, "memset needs the enum value to be 0");
  memset(new_states_p, 0, new_bucket_count * sizeof(E_slot_state));
  if (bucket_count) {
    memmove(new_states_p, m_states_p, bucket_count);
    m_states_p = new_states_p;
    rehash(bucket_count);
  } else {
    m_states_p = new_states_p;
  }
}

M_hash_table_t_
typename M_hash_table_c_::Iterator_t_ M_hash_table_c_::begin() const {
  if (m_count == 0) {
    return end();
  }
  Iterator_t_ it;
  it.m_ht = this;
  it.m_idx = 0;
  if (m_states_p[0] != e_slot_state_alive) {
    ++it;
  }
  return it;
}

M_hash_table_t_
typename M_hash_table_c_::Iterator_t_ M_hash_table_c_::end() const {
  Iterator_t_ it;
  it.m_ht = this;
  it.m_idx = m_bucket_count;
  return it;
}

M_hash_table_t_
Sz M_hash_table_c_::get_bucket_index(const T_key& key) const {
  return T_hash()(key) % m_bucket_count;
}

M_hash_table_t_
void M_hash_table_c_::rehash(int bucket_count) {
  // Updating states
  for (int i = 0; i < bucket_count; ++i) {
    E_slot_state& state = m_states_p[i];
    if (state == e_slot_state_alive) {
      state = e_slot_state_moving;
    } else if (state == e_slot_state_dead) {
      state = e_slot_state_free;
    }
  }

  // Actually moving
  for (int i = 0; i < bucket_count; ++i) {
    E_slot_state& old_state = m_states_p[i];
    if (old_state != e_slot_state_moving) {
      continue;
    }
    T_data& old_data = m_data_p[i];
    int starting_bucket_idx = get_bucket_index(old_data.key);
    for (int j = 0; j < m_bucket_count; ++j) {
      int new_bucket_idx = (starting_bucket_idx + j) % m_bucket_count;
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
