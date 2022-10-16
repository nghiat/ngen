//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;
struct Line_t_;

class Ttf_loader_t {
public:
  Ttf_loader_t(Allocator_t* allocator);
  bool init(const Path_t& path);
  void destroy();

  Dynamic_array_t<Line_t_> m_lines;
  float m_x_min;
  float m_y_min;
  float m_x_max;
  float m_y_max;
  U8* m_data;
  U16 m_width;
  U16 m_height;
};
