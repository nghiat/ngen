//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

// Provide methods to read bits value from a bytes array.
class Bit_stream_t {
public:
  bool init(const U8* data);

  // Get number from |bit_count| without moving the bit pointer.
  U64 read_lsb(Sip bit_count);
  U64 read_msb(Sip bit_count);

  void skip(Sip bit_count);

  // Same as |Read*| but also skip |bit_count|.
  U64 consume_lsb(Sip bit_count);
  U64 consume_msb(Sip bit_count);

  const U8* m_data;
  Sip m_index;
};
