//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/bit_stream.h"

U16 Bit_stream_t::consume_lsb(Sip bit_count) {
  U16 result = 0;
  Sip shift_count = 0;
  U8 remaining_bit_count = 8 - m_bit_index;
  while (bit_count > remaining_bit_count) {
    U8 v = m_data[m_byte_index];
    v = (v >> m_bit_index) & ((1 << remaining_bit_count) - 1);
    result |= v << shift_count;
    shift_count += remaining_bit_count;
    m_bit_index = 0;
    ++m_byte_index;
    bit_count -= remaining_bit_count;
    remaining_bit_count = 8;
  }
  U8 v = m_data[m_byte_index];
  v = (v >> m_bit_index) & ((1 << bit_count) - 1);
  result |= v << shift_count;
  m_byte_index += (m_bit_index + bit_count) / 8;
  m_bit_index = (m_bit_index + bit_count) % 8;
  return result;
}

U16 Bit_stream_t::consume_msb(Sip bit_count) {
  U16 result = 0;
  U8 remaining_bit_count = 8 - m_bit_index;
  if (m_cache_msb_byte_index != m_byte_index) {
    cache_msb_();
  }
  while (bit_count >= remaining_bit_count) {
    result <<= 8 - m_bit_index;
    result |= m_cache_msb & ((1 << remaining_bit_count) - 1);
    m_bit_index = 0;
    ++m_byte_index;
    bit_count -= remaining_bit_count;
    remaining_bit_count = 8;
    cache_msb_();
  }
  if (bit_count) {
    result <<= bit_count;
    result |= (m_cache_msb >> (8 - bit_count - m_bit_index)) & ((1 << bit_count) - 1);
    m_bit_index = m_bit_index + bit_count;
  }
  return result;
}

void Bit_stream_t::skip(Sip bit_count) {
  m_byte_index += (m_bit_index + bit_count) / 8;
  m_bit_index = (m_bit_index + bit_count) % 8;
}

U8 Bit_stream_t::cache_msb_() {
  U8 v = m_data[m_byte_index];
  v = (v & 0xF0) >> 4 | (v & 0x0F) << 4;
  v = (v & 0xCC) >> 2 | (v & 0x33) << 2;
  v = (v & 0xAA) >> 1 | (v & 0x55) << 1;
  m_cache_msb_byte_index = m_byte_index;
  m_cache_msb = v;
  return m_cache_msb;
}
