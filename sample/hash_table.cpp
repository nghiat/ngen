#include "core/core_init.h"
#include "core/hash_table.inl"
#include "core/hash_table2.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/mono_time.h"
#include <unordered_map>

template <typename T>
F64 time_func(T f) {
  S64 t0 = mono_time_now();
  f();
  return mono_time_to_s(mono_time_now() - t0);
}

int main(int argc, const char** argv) {
  core_init(M_os_txt("hash_table.log"));
  int num = 1000000;
  int lookup_loop = 100;

  std::unordered_map<int, int, Hash_t<int>> std_hash_table;
  std_hash_table.reserve(num);
  F64 std_hash_table_insert_time = time_func([&]() {
    for (int i = 0; i < num; ++i) {
      std_hash_table[i] = i;
    }
  });
  F64 std_hash_table_lookup_time = time_func([&]() {
    for (int j = 0; j < lookup_loop; ++j) {
      for (int i = 0; i < num; ++i) {
        std_hash_table[i];
      }
    }
  });
  M_logi("std::unordered_map: ");
  // M_logi("  reserve time: %f s", mono_time_to_s(t5 - t4));
  M_logi("  insert time: %f s", std_hash_table_insert_time);
  M_logi("  lookup time: %f s", std_hash_table_lookup_time);

  Linear_allocator_t<> allocator("hash_table_allocator");
  allocator.init();
  Hash_map<int, int> hash_table;
  hash_table.init(&allocator);
  hash_table.reserve(num);
  F64 hash_table_insert_time = time_func([&]() {
    for (int i = 0; i < num; ++i) {
      hash_table[i] = i;
    }
  });
  F64 hash_table_lookup_time = time_func([&]() {
    for (int j = 0; j < lookup_loop; ++j) {
      for (int i = 0; i < num; ++i) {
        hash_table[i];
      }
    }
  });

  M_logi("My hash_table: ");
  // M_logi("  reserve time: %f s", );
  M_logi("  insert time: %f s", hash_table_insert_time);
  M_logi("  lookup time: %f s", hash_table_lookup_time);

  Linear_allocator_t<> allocator2("hash_table2_allocator");
  allocator2.init();
  Hash_map2<int, int> hash_table2;
  hash_table2.init(&allocator2);
  hash_table2.reserve(num);
  F64 hash_table2_insert_time = time_func([&]() {
    for (int i = 0; i < num; ++i) {
      hash_table2[i] = i;
    }
  });
  F64 hash_table2_lookup_time = time_func([&]() {
    for (int j = 0; j < lookup_loop; ++j) {
      for (int i = 0; i < num; ++i) {
        hash_table2[i];
      }
    }
  });

  M_logi("My hash_table2: ");
  // M_logi("  reserve time: %f s", );
  M_logi("  insert time: %f s", hash_table2_insert_time);
  M_logi("  lookup time: %f s", hash_table2_lookup_time);

  for (int i = 0; i < num; ++i) {
    M_check(hash_table[i] == std_hash_table[i] && hash_table2[i] == std_hash_table[i]);
  }

  // int max_probe = 0;
  // const int c_probe_limit = 1000;
  // int probes[c_probe_limit] = {};
  // for (int i = 0; i < hash_table.m_buckets.len(); ++i) {
  //   int probe = hash_table.m_buckets[i].len;
  //   if (probe > max_probe) {
  //     max_probe = probe;
  //   }
  //   ++probes[probe];
  // }
  // M_logi("  max probe: %d", max_probe);
  // for (int i = 0; i < c_probe_limit; ++i) {
  //   if (probes[i]) {
  //     M_logi("  probe %d count: %d", i, probes[i]);
  //   }
  // }
  return 0;
}
