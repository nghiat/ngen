//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

class Dynamic_lib {
public:
  bool dl_open(const char* name);
  void dl_close();
  void* dl_get_proc(const char* name);

  void* m_handle;
};
