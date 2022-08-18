//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/png.h"

#include "test/test.h"

void bit_stream_test() {
  {
    U8 data[5];
    data[0] = 0b11001100;
    data[1] = 0b10101010;
    data[2] = 0b00001111;
    data[3] = 0b11111110;
    data[4] = 0b00000001;
    Bit_stream_t bs(data, 5);
    M_test(bs.consume_lsb(1) == 0b0);
    M_test(bs.consume_lsb(2) == 0b10);
    M_test(bs.consume_lsb(3) == 0b001);
    M_test(bs.consume_lsb(4) == 0b1011);
    M_test(bs.consume_lsb(5) == 0b01010);
    M_test(bs.consume_lsb(16) == 0b1111110000011111);
    M_test(bs.consume_lsb(9) == 0b000000011);
  }
  {
    U8 data[5];
    data[0] = 0b11001100;
    data[1] = 0b10101010;
    data[2] = 0b00001111;
    data[3] = 0b11111110;
    data[4] = 0b00000001;
    Bit_stream_t bs(data, 5);
    M_test(bs.consume_msb(1) == 0b0);
    M_test(bs.consume_msb(2) == 0b01);
    M_test(bs.consume_msb(3) == 0b100);
    M_test(bs.consume_msb(4) == 0b1101);
    M_test(bs.consume_msb(5) == 0b01010);
    M_test(bs.consume_msb(16) == 0b1111100000111111);
    M_test(bs.consume_msb(9) == 0b110000000);
  }
}
