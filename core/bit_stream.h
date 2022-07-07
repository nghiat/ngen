//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/types.h"

// Provide methods to read bits value from a bytes array.
class Bit_stream_t {
public:
  bool init(const U8* data);

  U16 consume_lsb(Sip bit_count);
  U16 consume_msb(Sip bit_count);
  void skip(Sip bit_count);

  const U8* m_data;
  U8 m_cache_msb;
  Sip m_cache_msb_byte_index = -1;
  Sip m_byte_index;
  Sip m_bit_index;

private:
  U8 cache_msb_();
};
