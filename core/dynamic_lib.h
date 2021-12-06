//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

class Dynamic_lib {
public:
  bool open(const char* name);
  void close();
  void* get_proc(const char* name);

  void* m_handle;
};
