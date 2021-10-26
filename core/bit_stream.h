//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

// Provide methods to read bits value from a bytes array.
class Bit_stream {
public:
  bool bs_init(const U8* data);

  // Get number from |num_of_bits| without moving the bit pointer.
  U64 bs_read_lsb(Sip num_of_bits);
  U64 bs_read_msb(Sip num_of_bits);

  void bs_skip(Sip num_of_bits);

  // Same as |Read*| but also skip |num_of_bits|.
  U64 bs_consume_lsb(Sip num_of_bits);
  U64 bs_consume_msb(Sip num_of_bits);

  const U8* m_data;
  Sip m_index;
};
