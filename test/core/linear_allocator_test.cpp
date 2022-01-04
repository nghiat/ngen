//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/linear_allocator.inl"
#include "core/utils.h"
#include "test/test.h"

void linear_allocator_test() {
  // Multiple 1 byte allocation
  {
    // In this test we alloc multiple 1 bytes and assign different values for
    // each bytes.
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    constexpr int c_num = 64;
    U8* array[c_num];
    for (size_t i = 0; i < c_num; ++i) {
      array[i] = (U8*)allocator.alloc(1);
      *array[i] = i;
    }
    int sum = 0;
    for (size_t i = 0; i < c_num; ++i) {
      sum += *array[i];
    }
    int correct_sum = c_num * (c_num - 1) / 2;
    M_test(sum == correct_sum);
  }

  // Zero size
  {
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    size_t current_used_size = allocator.m_used_size;
    M_test(allocator.alloc(0) == NULL);
    M_test(allocator.aligned_alloc(0, 2) == NULL);
    M_test(allocator.m_used_size == current_used_size);
  }

  // Aligned Allocation
  {
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());

    allocator.alloc(1);
    constexpr size_t c_alignment = 512;
    void* aligned_pointer = allocator.aligned_alloc(c_alignment, c_alignment);
    M_test(aligned_pointer);
    M_test((Sz)aligned_pointer % c_alignment == 0);
  }

  // Non power of two alignment and zero alignment
  {
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    M_test(allocator.aligned_alloc(1, 0) == NULL);
    M_test(allocator.aligned_alloc(1, 3) == NULL);
  }

  // alloc & free
  {
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    size_t c_initial_used_size = allocator.m_used_size;
    S64* qword1 = (int64_t*)allocator.alloc(sizeof(S64));
    S64* qword2 = (int64_t*)allocator.alloc(sizeof(S64));
    *qword1 = 1111;
    *qword2 = 2222;
    M_test(*qword1 == 1111);
    M_test(*qword2 == 2222);

    allocator.free(qword2);
    S64* qword3 = (S64*)allocator.alloc(sizeof(S64));
    *qword3 = 3333;
    M_test(qword2 == qword3);
    M_test(*qword1 == 1111);
    M_test(*qword3 == 3333);

    allocator.free(qword3);
    S64* qword4 = (S64*)allocator.aligned_alloc(sizeof(S64), 64);
    *qword4 = 4444;
    M_test(*qword1 == 1111);
    M_test(*qword4 == 4444);

    allocator.free(qword4);
    S64* qword5 = (S64*)allocator.aligned_alloc(sizeof(S64), 64);
    *qword5 = 5555;
    M_test(qword4 == qword5);
    M_test(*qword1 == 1111);
    M_test(*qword5 == 5555);

    // Cleaup
    allocator.free(qword5);
    allocator.free(qword1);
    M_test(allocator.m_used_size == c_initial_used_size);
  }

  // alloc bigger than the stack page
  {
    Linear_allocator<128> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    void* p = allocator.alloc(1);
    void* p2 = allocator.alloc(1024);
    M_test(p);
    M_test(p2);
    p = allocator.realloc(p, 1024);
    M_test(p);
  }

  // realloc
  {
    Linear_allocator<> allocator("test");
    allocator.init();
    M_scope_exit(allocator.destroy());
    void* p1 = allocator.alloc(128);
    void* p2 = allocator.alloc(128);
    /// Invalid argument.
    M_test(allocator.realloc(nullptr, 0) == nullptr);
    M_test(allocator.realloc(p2, 0) == nullptr);
    /// Not the top of the stack pointer.
    p1 = allocator.realloc(p1, 256);
    M_test(p1);

    // Top of the stack pointer.
    M_test(allocator.realloc(p1, 128) == p1);
    M_test(allocator.realloc(p1, 256) == p1);

    // Previous top of the stack pointer
    allocator.free(p1);
    M_test(allocator.realloc(p2, 64) == p2);
    M_test(allocator.realloc(p2, 256) == p1);
  }
}
