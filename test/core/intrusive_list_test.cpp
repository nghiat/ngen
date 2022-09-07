//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/intrusive_list.inl"

#include "test/test.h"

struct Node_t : public Intrusive_list_t<Node_t> {
  int a = 0;
};

void intrusive_list_test() {
  Intrusive_list_t<Node_t> list;
  list.init_head();
  constexpr int c_node_count = 10;
  Node_t nodes[c_node_count];
  for (int i = 0; i < c_node_count; ++i) {
    nodes[i].a = i;
    list.append(&nodes[i]);
  }
  int sum = 0;
  for (auto node : nodes) {
    sum += node.a;
  }
  M_test(sum == (c_node_count - 1) * c_node_count / 2);
}
