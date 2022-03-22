//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/log.h"
#include "core/reflection/reflection.h"
#include "sample/test_reflection_class.h"

int main() {
  core_init(M_txt("reflection.log"));
  Class_info_t* test_class = get_class_info<Reflected_class_t_>();
  M_logi("%s", test_class->m_class_name.m_p);
  M_logi("Fields:");
  for (int i = 0; i < test_class->m_fields.len(); ++i) {
    M_logi("\t%s", test_class->m_fields[i].m_p);
  }
  return 0;
}
