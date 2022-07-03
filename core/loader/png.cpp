//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

// hthtps://www.w3.org/TR/2003/REC-PNG-20031110
// https://www.ietf.org/rfc/rfc1951.txt

#include "core/loader/png.h"

#include "core/allocator.h"
#include "core/bit_stream.h"
#include "core/dynamic_array.h"
#include "core/file.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/os.h"
#include "core/utils.h"

#include <stdlib.h>

#if M_os_is_win()
#  define M_bswap32_(x) _byteswap_ulong(x)
#elif M_os_is_linux()
#  include <byteswap.h>
#  define M_bswap32_(x) bswap_32(x)
#endif

#define FOURCC_(cc) (cc[0] | cc[1] << 8 | cc[2] << 16 | cc[3] << 24)

// From 0 - 15
static const int gc_max_code__len_ = 16;
static const int gc_max_code_ = 286 + 30;
static const int gc_png_sig_len_ = 8;

static const U8 gc_png_signature_[gc_png_sig_len_] = {137, 80, 78, 71, 13, 10, 26, 10};

static const int gc_len_bases_[] = {3,  4,  5,  6,   7,   8,   9,   10,  11, 13,
                                    15, 17, 19, 23,  27,  31,  35,  43,  51, 59,
                                    67, 83, 99, 115, 131, 163, 195, 227, 258};

static const int gc_len_extra_bits_[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                         1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                         4, 4, 4, 4, 5, 5, 5, 5, 0};

static const int gc_dist_bases_[] = {
    1,    2,    3,    4,    5,    7,    9,    13,    17,    25,
    33,   49,   65,   97,   129,  193,  257,  385,   513,   769,
    1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

static const int gc_dist_extra_bits_[] = {0, 0, 0,  0,  1,  1,  2,  2,  3,  3,
                                          4, 4, 5,  5,  6,  6,  7,  7,  8,  8,
                                          9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

// Structure saves codes of specific length.
// |codes| contains codes of a specific length in an increasing order.
// |min| is the smallest codes of that length.
// |count| is the number of codes.
struct Codes_for_length_t_ {
  U16 codes[gc_max_code_];
  U16 min;
  U16 count;
};

// |cfl| is a pointer to array of where the index is the length and value is the
// codes for that length.
struct Alphabet_t_ {
  Codes_for_length_t_* cfl;
  U8 min_len;
  U8 max_len;
};

// Build an alphabet based on lengths of codes from 0 to |count| - 1.
static void build_alphabet_(U8* lens, int count, Alphabet_t_* alphabet) {
  U8 len_counts[gc_max_code__len_] = {};
  for (int i = 0; i < count; ++i) {
    if (lens[i]) {
      alphabet->cfl[lens[i]].codes[len_counts[lens[i]]] = i;
      len_counts[lens[i]]++;
    }
  }
  for (int i = 0; i < gc_max_code__len_; ++i) {
    if (len_counts[i]) {
      alphabet->min_len = i;
      break;
    }
  }
  for (U8 i = gc_max_code__len_ - 1; i > 0; --i) {
    if (len_counts[i]) {
      alphabet->max_len = i;
      break;
    }
  }
  int smallest_code = 0;
  for (U8 i = alphabet->min_len; i <= alphabet->max_len; ++i) {
    smallest_code = smallest_code;
    alphabet->cfl[i].min = smallest_code;
    alphabet->cfl[i].count = len_counts[i];
    smallest_code = (smallest_code + len_counts[i]) << 1;
  }
}

static int decode_(Bit_stream_t* bs, const Alphabet_t_* c_alphabet) {
  int code = bs->consume_msb(c_alphabet->min_len);
  for (U8 i = c_alphabet->min_len; i <= c_alphabet->max_len; ++i) {
    int delta_to_min = code - c_alphabet->cfl[i].min;
    if (delta_to_min < c_alphabet->cfl[i].count) {
      return c_alphabet->cfl[i].codes[delta_to_min];
    }
    code = code << 1 | bs->consume_msb(1);
  }
  M_logf("Can't decode_");
  return -1;
}

static void decode_len_and_dist_(U8** o_deflated, int len_code, Bit_stream_t* bs, const Alphabet_t_* c_dist_alphabet) {
  int len_idx = len_code % 257;
  int len_base = gc_len_bases_[len_idx];
  int len_extra_bits = gc_len_extra_bits_[len_idx];
  int len = len_base + bs->consume_lsb(len_extra_bits);
  int dist_idx = c_dist_alphabet ? decode_(bs, c_dist_alphabet) : bs->consume_lsb(5);
  int dist_base = gc_dist_bases_[dist_idx];
  int dist_extra_bits = gc_dist_extra_bits_[dist_idx];
  int dist = dist_base + bs->consume_lsb(dist_extra_bits);
  U8* copy = *o_deflated - dist;
  for (int i = 0; i < len; ++i) {
    *(*o_deflated)++ = *copy++;
  }
}

static int paeth_(int a, int b, int c) {
  int p = a + b - c;
  int pa = abs(p - a);
  int pb = abs(p - b);
  int pc = abs(p - c);
  if (pa <= pb && pa <= pc) {
    return a;
  } else if (pb <= pc) {
    return b;
  }
  return c;
}

bool Png_loader_t::init(Allocator_t* allocator, const Path_t& path) {
  m_allocator = allocator;

  Linear_allocator_t<> temp_allocator("PNG_loader_temp_allocator");
  temp_allocator.init();
  M_scope_exit(temp_allocator.destroy());
  Dynamic_array_t<U8> data = File_t::read_whole_file_as_text(&temp_allocator, path.m_path);
  M_check_log_return_val(!memcmp(&data[0], &gc_png_signature_[0], gc_png_sig_len_), false, "Invalid PNG signature");
  for (int i = gc_png_sig_len_; i < data.len();) {
    int data_len = M_bswap32_(*((int*)(&data[0] + i)));
    i += 4;
    const U8* chunk_it = &data[0] + i;
    const int chunk_type = *((int*)(&data[0] + i));
    i += 4;
    U8* p = &data[0] + i;
    i += data_len;
    // U8* cRC = it;
    i += 4;
    switch (chunk_type) {
    case FOURCC_("IHDR"): {
      M_check_return_val(data_len == 13, false);
      m_width = M_bswap32_(*(int*)p);
      p += 4;
      m_height = M_bswap32_(*(int*)p);
      p += 4;
      m_bit_depth = *p++;
      U8 color_type = *p++;
      switch (color_type) {
      case 0:
        // Grey scale
        m_values_per_pixel = 1;
        break;
      case 2:
        // RGB
        m_values_per_pixel = 3;
        break;
      case 3:
        M_unimplemented();
        break;
      case 4:
        // Grey scale with alpha
        m_values_per_pixel = 2;
      case 6:
        // RGBA
        m_values_per_pixel = 4;
        break;
      default:
        M_logf_return_val(false, "Invalid color type");
      }
      U8 compression_method = *p++;
      M_check_log_return_val(!compression_method, false, "Invalid compression method");
      U8 filter_method = *p++;
      M_check_log_return_val(!filter_method, false, "Invalid filter method");
      const U8 interlace_method = *p++;
      M_check_log_return_val(!interlace_method, false, "Invalid interlace method");
      break;
    }
    case FOURCC_("PLTE"): {
      break;
    }
    case FOURCC_("IDAT"): {
      Bit_stream_t bs;
      bs.init(p);
      // 2 bytes of zlib header.
      U32 zlib_compress_method = bs.consume_lsb(4);
      M_check_log_return_val(zlib_compress_method == 8, false, "Invalid zlib compression method");
      U32 zlib_compress_info = bs.consume_lsb(4);
      M_check_log_return_val((p[0] * 256 + p[1]) % 31 == 0, false, "Invalid FCHECK bits");
      bs.skip(5);
      U8 fdict = bs.consume_lsb( 1);
      U8 flevel = bs.consume_lsb(2);

      U8* deflated_data = (U8*)temp_allocator.alloc((m_width + 1) * m_height * m_values_per_pixel);
      U8* deflated_p = deflated_data;
      while (true) {
        // 3 header bits
        const U8 bfinal = bs.consume_lsb(1);
        const U8 ctype = bs.consume_lsb(2);
        if (ctype == 1) {
          // Fixed Huffman.
          for (;;) {
            int code;
            code = bs.consume_msb(7);
            if (code >= 0 && code <= 23) {
              code += 256;
              if (code == 256) {
                break;
              }
            } else {
              code = code << 1 | bs.consume_msb(1);
              if (code >= 48 && code <= 191) {
                *deflated_p++ = code - 48;
                continue;
              } else if (code >= 192 && code <= 199) {
                code += 88;
              } else {
                code = code << 1 | bs.consume_msb(1);
                M_check_log_return_val(code >= 400 && code <= 511, false, "Can't decode_ fixed Huffman");
                *deflated_p++ = code - 256;
                continue;
              }
            }
            decode_len_and_dist_(&deflated_p, code, &bs, NULL);
          }
        } else if (ctype == 2) {
          // Dynamic Huffman.
          int hlit = bs.consume_lsb(5) + 257;
          int hdist = bs.consume_lsb(5) + 1;
          int hclen = bs.consume_lsb(4) + 4;
          U8 len_of_len[19] = {};
          const int c_len_alphabet[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
          for (int j = 0; j < hclen; ++j) {
            len_of_len[c_len_alphabet[j]] = bs.consume_lsb(3);
          }
          Codes_for_length_t_ code_lens_cfl[8] = {};
          Alphabet_t_ code_len_alphabet = {code_lens_cfl, 0, 0};
          build_alphabet_(len_of_len, 19, &code_len_alphabet);
          int index = 0;
          U8 lit_and_dist_lens[gc_max_code_] = {};
          while (index < hlit + hdist) {
            int code_len = decode_(&bs, &code_len_alphabet);
            if (code_len < 16) {
              lit_and_dist_lens[index++] = code_len;
            } else {
              U8 repeat_count;
              U8 repeated_val = 0;
              if (code_len == 16) {
                repeat_count = bs.consume_lsb(2) + 3;
                repeated_val = lit_and_dist_lens[index - 1];
              } else if (code_len == 17) {
                repeat_count = bs.consume_lsb(3) + 3;
              } else if (code_len == 18) {
                repeat_count = bs.consume_lsb(7) + 11;
              }
              memset(lit_and_dist_lens + index, repeated_val, repeat_count);
              index += repeat_count;
            }
            M_check_log_return_val(index <= hlit + hdist, false, "Can't decode_ literal and length alphabet, overflowed");
          }
          M_check_log_return_val(lit_and_dist_lens[256], false, "Symbol 256 can't have length of 0");
          Codes_for_length_t_ lit_or_len_cfl[gc_max_code__len_] = {};
          Alphabet_t_ lit_or_len_alphabet = {lit_or_len_cfl, 0, 0};
          build_alphabet_(lit_and_dist_lens, hlit, &lit_or_len_alphabet);
          Codes_for_length_t_ dist_cfl[gc_max_code__len_] = {};
          Alphabet_t_ dist_alphabet = {dist_cfl, 0, 0};
          build_alphabet_(lit_and_dist_lens + hlit, hdist, &dist_alphabet);
          for (;;) {
            int lit_or_len_code = decode_(&bs, &lit_or_len_alphabet);
            if (lit_or_len_code == 256) {
              break;
            }
            if (lit_or_len_code < 256) {
              *deflated_p++ = lit_or_len_code;
              continue;
            }
            decode_len_and_dist_(&deflated_p, lit_or_len_code, &bs, &dist_alphabet);
          }
        }
        if (bfinal) {
          break;
        }
      }
      m_data = (U8*)m_allocator->alloc(m_width * m_height * m_values_per_pixel);
      int bytes_per_deflated_row = m_values_per_pixel * m_width + 1;
      int bytes_per_data_row = m_values_per_pixel * m_width;
      for (int r = 0; r < m_height; ++r) {
        const U8 filter_type = deflated_data[r * bytes_per_deflated_row];
        const int data_offset = r * bytes_per_data_row;
        const int deflated_offset = r * bytes_per_deflated_row + 1;
        U8* a = &m_data[r * bytes_per_data_row];
        U8* b = NULL;
        if (r) {
          b = &m_data[(r - 1) * bytes_per_data_row];
        }
        U8* c = NULL;
        if (r) {
          c = &m_data[(r - 1) * bytes_per_data_row];
        }
        switch (filter_type) {
        case 0: {
          memcpy(m_data + data_offset, deflated_data + deflated_offset, bytes_per_data_row);
        } break;
        case 1: {
          for (int j = 0; j < m_values_per_pixel; ++j) {
            m_data[data_offset + j] = deflated_data[deflated_offset + j];
          }
          for (int j = m_values_per_pixel; j < bytes_per_data_row; ++j) {
            m_data[data_offset + j] = deflated_data[deflated_offset + j] + *a++;
          }
        } break;
        case 2: {
          if (!r) {
            for (int j = 0; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j];
            }
          } else {
            for (int j = 0; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *b++;
            }
          }
        } break;
        case 3: {
          if (!r) {
            for (int j = 0; j < m_values_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j];
            }
            for (int j = m_values_per_pixel; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *a++ / 2;
            }
          } else {
            for (int j = 0; j < m_values_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *b++ / 2;
            }
            for (int j = m_values_per_pixel; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + ((int)(*a++) + (int)(*b++)) / 2;
            }
          }
        } break;
        case 4: {
          if (!r) {
            for (int j = 0; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j];
            }
          } else {
            for (int j = 0; j < m_values_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *b++;
            }
            for (int j = m_values_per_pixel; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + paeth_(*(a++), *(b++), *(c++));
            }
          }
        } break;
        default:
          M_logf_return_val(false, "Invalid filter method");
        }
      }
    } break;
    case FOURCC_("IEND"):
      break;
    }
  }
  return true;
}

void Png_loader_t::destroy() {
  m_allocator->free(m_data);
}
