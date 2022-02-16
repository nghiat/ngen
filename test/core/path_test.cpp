//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/path.h"

#include "test/test.h"

void path_test() {
  Path8_t p;
  M_test(p.m_path_str.m_length == 0);
  M_test(p.m_path_str.m_capacity == M_max_path_len);

  p = Path8_t("a.cpp");
  M_test(p.m_path_str.m_length == 5);
  M_test(Path8_t::from_char("a.cpp").equals(p));
  M_test(p.get_path8().equals(p));

  M_test(!p.is_dir());
  M_test(p.is_file());
  M_test(Path8_t("a/").is_dir());
  M_test(!Path8_t("a/").is_file());
}
