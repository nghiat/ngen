//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/ng_types.h"
#include "core/linear_allocator.h"
#include "core/math/vec2.h"

struct Line_info_t_;

class Quake_console_t {
public:
  bool init(F32 width, F32 height);
  void destroy();
  void append_codepoint(U32 cp);
  void append_string(const char* str);

  Linear_allocator_t<> m_codepoint_allocator = Linear_allocator_t<>("console_codepoint_allocator");
  Dynamic_array_t<U32> m_codepoint_buffer;
  Linear_allocator_t<> m_line_indices_allocator = Linear_allocator_t<>("console_line_indices_allocator");
  Dynamic_array_t<Line_info_t_> m_codepoint_line_indices_buffer;
  Linear_allocator_t<> m_f_allocator = Linear_allocator_t<>("console_f_allocator");
  Dynamic_array_t<U8> m_f_buf;
  V2_t m_rect[6];
  F32 m_width;
  F32 m_height;
};
