//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/bit_stream.h"

bool Bit_stream::init(const U8* data) {
  m_data = data;
  m_index = 0;
  return true;
}

U64 Bit_stream::read_lsb(Sip bit_count) {
  U64 result = 0;
  Sip temp_idx = m_index;
  for (Sip i = 0; i < bit_count; ++i, temp_idx++) {
    Sip byte_count = temp_idx / 8;
    Sip bit_in_byte = temp_idx % 8;
    result |= ((m_data[byte_count] >> bit_in_byte) & 1) << i;
  }
  return result;
}

U64 Bit_stream::read_msb(Sip bit_count) {
  U64 result = 0;
  Sip temp_idx = m_index;
  for (Sip i = 0; i < bit_count; ++i, temp_idx++) {
    Sip byte_count = temp_idx / 8;
    Sip bit_in_byte = temp_idx % 8;
    result |= ((m_data[byte_count] >> bit_in_byte) & 1)
              << (bit_count - 1 - i);
  }
  return result;
}

void Bit_stream::skip(Sip bit_count) {
  m_index += bit_count;
}

U64 Bit_stream::consume_lsb(Sip bit_count) {
  U64 result = read_lsb(bit_count);
  skip(bit_count);
  return result;
}

U64 Bit_stream::consume_msb(Sip bit_count) {
  U64 result = read_msb(bit_count);
  skip(bit_count);
  return result;
}
