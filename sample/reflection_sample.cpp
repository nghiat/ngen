//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/log.h"
#include "core/reflection/reflection.h"
#include "sample/test_reflection_class.h"

int main() {
  core_init(M_os_txt("reflection.log"));
  Class_info_t* test_class = get_class_info<Reflected_class_t_>();
  M_logi("%s", test_class->m_class_name);
  return 0;
}
