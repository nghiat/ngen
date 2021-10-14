//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/bit_stream.h"

bool BitStream::bs_init(const U8* data) {
  m_data = data;
  m_index = 0;
  return true;
}

U64 BitStream::bs_read_lsb(SIP num_of_bits) {
  U64 result = 0;
  SIP temp_idx = m_index;
  for (SIP i = 0; i < num_of_bits; ++i, temp_idx++) {
    SIP bytes_num = temp_idx / 8;
    SIP bit_in_byte = temp_idx % 8;
    result |= ((m_data[bytes_num] >> bit_in_byte) & 1) << i;
  }
  return result;
}

U64 BitStream::bs_read_msb(SIP num_of_bits) {
  U64 result = 0;
  SIP temp_idx = m_index;
  for (SIP i = 0; i < num_of_bits; ++i, temp_idx++) {
    SIP bytes_num = temp_idx / 8;
    SIP bit_in_byte = temp_idx % 8;
    result |= ((m_data[bytes_num] >> bit_in_byte) & 1)
              << (num_of_bits - 1 - i);
  }
  return result;
}

void BitStream::bs_skip(SIP num_of_bits) {
  m_index += num_of_bits;
}

U64 BitStream::bs_consume_lsb(SIP num_of_bits) {
  U64 result = bs_read_lsb(num_of_bits);
  bs_skip(num_of_bits);
  return result;
}

U64 BitStream::bs_consume_msb(SIP num_of_bits) {
  U64 result = bs_read_msb(num_of_bits);
  bs_skip(num_of_bits);
  return result;
}
