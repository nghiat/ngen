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

U64 Bit_stream::read_lsb(Sip num_of_bits) {
  U64 result = 0;
  Sip temp_idx = m_index;
  for (Sip i = 0; i < num_of_bits; ++i, temp_idx++) {
    Sip bytes_num = temp_idx / 8;
    Sip bit_in_byte = temp_idx % 8;
    result |= ((m_data[bytes_num] >> bit_in_byte) & 1) << i;
  }
  return result;
}

U64 Bit_stream::read_msb(Sip num_of_bits) {
  U64 result = 0;
  Sip temp_idx = m_index;
  for (Sip i = 0; i < num_of_bits; ++i, temp_idx++) {
    Sip bytes_num = temp_idx / 8;
    Sip bit_in_byte = temp_idx % 8;
    result |= ((m_data[bytes_num] >> bit_in_byte) & 1)
              << (num_of_bits - 1 - i);
  }
  return result;
}

void Bit_stream::skip(Sip num_of_bits) {
  m_index += num_of_bits;
}

U64 Bit_stream::consume_lsb(Sip num_of_bits) {
  U64 result = read_lsb(num_of_bits);
  skip(num_of_bits);
  return result;
}

U64 Bit_stream::consume_msb(Sip num_of_bits) {
  U64 result = read_msb(num_of_bits);
  skip(num_of_bits);
  return result;
}
