//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/gpu/gpu.h"
#include "core/path.h"
#include "core/types.h"

struct Allocator_t;

// Provide methods to read bits value from a bytes array.
class Bit_stream_t {
public:
  Bit_stream_t(const U8* data, int len) : m_data(data), m_len(len) { cache_(); }

  U16 peek_lsb(Sip bit_count);
  U16 consume_lsb(Sip bit_count);
  U16 consume_msb(Sip bit_count);
  void skip(Sip bit_count);

  const U8* m_data = NULL;
  Sip m_len = 0;
  U32 m_cache_lsb;
  Sip m_byte_index = 0;
  Sip m_bit_index = 0;
private:
  void cache_();
};

struct Png_loader_t {
public:
  bool init(Allocator_t* allocator, const Path_t& path);
  void destroy();

  Allocator_t* m_allocator;
  U8* m_data = NULL;
  U32 m_width = 0;
  U32 m_height = 0;
  U8 m_bit_depth = 0;
  U8 m_components_per_pixel = 0;
  U8 m_bytes_per_pixel = 0;
  E_format m_format;
};
