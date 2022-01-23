//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/utils.h"

#include "test/test.h"

static int g_count_ = 0;

void utils_test() {
  int count = 500;
  for (int i = 0; i < count; ++i) {
    M_scope_exit(++g_count_);
  }
  M_test(g_count_ == count);
}
