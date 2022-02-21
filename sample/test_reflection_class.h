//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/reflection/reflection.h"
#include "sample/test_reflection_class.reflection.h"

class R_class Reflected_class_t_ {
public:
  R_method
  void reflected_method() {}

  R_field
  int reflected_field;
};
