//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/hash_table.inl"

#include "core/free_list_allocator.h"
#include "test/test.h"

void hash_map_test() {
  Free_list_allocator_t allocator("hash_map_test_allocator", 1024000);
  allocator.init();
  M_scope_exit(allocator.destroy());

  Sz empty_allocator_used_size = allocator.m_used_size;

  {
    {
      constexpr int c_count = UINT8_MAX - 1;
      Hash_map<int, int> u8_map;
      u8_map.init(&allocator);
      for (uint8_t i = 0; i < c_count; ++i) {
        u8_map[i] = i;
      }
      bool ok = true;
      for (int i = 0; i < c_count; ++i) {
        int* v = u8_map.find(i);
        if (v && *v != i) {
          ok = false;
          break;
        }
      }
      M_test(ok);
      M_test(u8_map.m_count == c_count);
      M_test(u8_map.find(c_count + 1) == nullptr);
      u8_map.destroy();
    }

    M_test(allocator.m_used_size == empty_allocator_used_size);
  }
  {
    Hash_map<int, int> map;
    map.init(&allocator);
    constexpr int c_count = 500;
    int correct_sum = 0;
    for (int i = 0; i < c_count; ++i) {
      int val = rand() % 100 + 100 * i; // avoid rand() collision
      correct_sum += 2 * val;
      map[val] = val;
    }
    int sum = 0;
    int it_count = 0;
    for (auto& it : map) {
      ++it_count;
      sum += it.key + it.value;
    }
    map.destroy();
    M_test(it_count == c_count);
    M_test(sum == correct_sum);
    M_test(allocator.m_used_size == empty_allocator_used_size);
  }
  // {
  //   hash_map_t<int, int> map;
  //   ht_init(&map, &allocator);
  //   ht_insert_or_get(&map, 1, 1);
  //   ht_insert_or_get(&map, 3, 3);
  //   ht_insert_or_get(&map, 2, 2);
  //   ht_insert_or_get(&map, 4, 4);
  //   ht_remove_key(&map, 2);
  //   ht_remove_key(&map, 1);
  //   ht_remove_key(&map, 4);
  //   ht_insert_or_get(&map, 1, -1);
  //   REQUIRE(ht_find(&map, 1)->value == -1);
  //   REQUIRE(ht_find(&map, 3)->value == 3);

  //   ht_remove_it(&map, ht_insert_or_get(&map, 5, 5).first);
  //   ht_remove_it(&map, ht_insert_or_get(&map, 6, 6).first);
  //   REQUIRE(map.length == 2);
  //   REQUIRE(ht_find(&map, 5) == end(map));
  //   REQUIRE(ht_find(&map, 6) == end(map));
  //   ht_destroy(&map);
  // }
}
