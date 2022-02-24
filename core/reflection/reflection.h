//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#define R_class __attribute__((annotate("reflected")))
#define R_field __attribute__((annotate("reflected")))
#define R_method __attribute__((annotate("reflected")))

class Class_info_t {
public:
  static const int sc_max_class_name_length = 64;
  char m_class_name[sc_max_class_name_length] = {};
};

template <typename T>
Class_info_t* get_class_info();
