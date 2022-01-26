//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/dynamic_array.inl"

#include "core/free_list_allocator.h"
#include "test/test.h"

void dynamic_array_test() {
  Free_list_allocator_t allocator("dynamic_array_test_allocator", 1024 * 1024 * 1024);
  allocator.init();
  {
    Dynamic_array_t<int> array;
    array.init(&allocator);
    M_test(allocator.m_used_size == 0);
  }

  {
    Dynamic_array_t<S8> s8_array;
    s8_array.init(&allocator);
    const int elem_count = 10;
    s8_array.reserve(elem_count);
    M_test(allocator.m_used_size == elem_count * sizeof(S8));

    // constexpr esz smallerElemsNum = 5;
    // s8_array.Reserve(smallerElemsNum);
    // REQUIRE(allocator.GetActualUsedSize() == smallerElemsNum * sizeof(ei8));

    // constexpr esz biggerElemsNum = 100;
    // s8_array.Reserve(biggerElemsNum);
    // REQUIRE(allocator.GetActualUsedSize() == biggerElemsNum * sizeof(ei8));

    // SECTION("Multiple times") {
    //   for (int i = 0; i < 1000; ++i) {
    //     s8_array.Reserve(smallerElemsNum);
    //     s8_array.Reserve(biggerElemsNum);
    //   }
    //   REQUIRE(s8_array.Capacity() == biggerElemsNum);
    // }
  }

  // {
  //   test::TestUnsignedType<eu8>(&allocator);
  //   test::TestUnsignedType<eu16>(&allocator);
  //   test::TestUnsignedType<eu32>(&allocator);
  //   test::TestUnsignedType<eu64>(&allocator);
  //   test::TestUnsignedType<ef32>(&allocator);
  //   test::TestUnsignedType<ef64>(&allocator);

  //   test::TestSignedType<ei8>(&allocator);
  //   test::TestSignedType<ei16>(&allocator);
  //   test::TestSignedType<ei32>(&allocator);
  //   test::TestSignedType<ei64>(&allocator);
  // }

  // {
  //   DynamicArray<eu8> array(&allocator);
  //   REQUIRE(array.IsEmpty());
  //   constexpr esz cCount = 10;
  //   for (esz i = 0; i < cCount; ++i)
  //     array.Append((eu8)i);
  //   REQUIRE(array.Length() == cCount);
  //   REQUIRE(array.Capacity() == cCount);
  //   REQUIRE_FALSE(array.IsEmpty());
  // }

  // {
  //   DynamicArray<int> array(&allocator);
  //   for (int i = 0; i < 1000; ++i)
  //     array.Append(i);

  //   array.RemoveAt(500);
  //   REQUIRE(array.Length() == 999);
  //   REQUIRE(array[499] == 499);
  //   REQUIRE(array[500] == 501);

  //   array.RemoveRange(0, 3);
  //   REQUIRE(array.Length() == 996);
  //   REQUIRE(array[0] == 3);

  //   array.RemoveAt(0);
  //   REQUIRE(array.Length() == 995);
  //   REQUIRE(array[0] == 4);
  // }

  // {
  //   constexpr esz cCount = 10;
  //   DynamicArray<eu8> array(&allocator);
  //   for (esz i = 0; i < cCount; ++i)
  //     array.Append(i);
  //   constexpr esz cSum = (cCount - 1) * cCount / 2;
  //   {
  //     esz sum = 0;
  //     for (eu8 i : array)
  //       sum += i;
  //     REQUIRE(sum == cSum);
  //   }
  //   {
  //     esz sum = 0;
  //     for (const eu8 i : array)
  //       sum += i;
  //     REQUIRE(sum == cSum);
  //   }
  //   {
  //     esz sum = 0;
  //     for (eu8& i : array) {
  //       sum += i;
  //     }
  //     REQUIRE(sum == cSum);
  //   }
  //   {
  //     esz sum = 0;
  //     for (const eu8& i : array)
  //       sum += i;
  //     REQUIRE(sum == cSum);
  //   }

  //   // Last of this test case.
  //   {
  //     bool ok = true;
  //     for (eu8& i : array)
  //       i = 0;
  //     for (eu8 i : array)
  //       if (i)
  //         ok = false;
  //     REQUIRE(ok);
  //   }
  // }
  // {
  //   struct Non_trivial_t {
  //     Non_trivial_t(int* n) : _number(n) {
  //     }
  //     ~Non_trivial_t() {
  //       *_number = 0;
  //     };
  //     int* _number;
  //   };
  //   int n = 1;
  //   {
  //     DynamicArray<Non_trivial_t> array(&allocator);
  //     array.Append(&n);
  //   }
  //   REQUIRE(n == 0);
  // }
  // {
  //   DynamicArray<eu8> array(&allocator);
  //   constexpr esz cNum = 8;
  //   esz oldAllocatorUsedSize = allocator.GetActualUsedSize();
  //   array.Resize(cNum);
  //   REQUIRE(array.Length() == 8);
  //   REQUIRE(allocator.GetActualUsedSize() ==
  //           oldAllocatorUsedSize + cNum * sizeof(eu8));
  // }
}
