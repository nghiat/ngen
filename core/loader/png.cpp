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

#define M_fast_bit_count 11

// From 0 - 15
static const int gc_max_code_len_ = 16;
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

struct Symbol_t_ {
  U16 val;
  U8 len;
};

// |cfl| is a pointer to array of where the index is the length and value is the
// codes for that length.
struct Alphabet_t_ {
  Codes_for_length_t_ cfls[gc_max_code_len_];
  // Since Huffman codes use prefix code, so each code has unique prefix.
  // But since reading msb is slow, we define |M_fast_bit_count| and reverse all the codes that are shorter or equal in length.
  // So the reversed codes will have unique endings. And we fill the remaining bits with all possible prefixes and use them as the indices for |fast_table|.
  // So when we read from a bit stream, we can just read |M_fast_bit_count| bits as the index.
  // But since most of the codes have shorter lengths, we've benchmarked |M_fast_bit_count| and pick the current value.
  Symbol_t_ fast_table[1 << M_fast_bit_count];
  U8 min_len;
  U8 max_len;
};

static U16 reverse16_(U16 n) {
  n = ((n & 0xAAAA) >>  1) | ((n & 0x5555) << 1);
  n = ((n & 0xCCCC) >>  2) | ((n & 0x3333) << 2);
  n = ((n & 0xF0F0) >>  4) | ((n & 0x0F0F) << 4);
  n = ((n & 0xFF00) >>  8) | ((n & 0x00FF) << 8);
  return n;
}

// Build an alphabet based on lengths of codes from 0 to |count| - 1.
static void build_alphabet_(U8* lens, int count, Alphabet_t_* alphabet) {
  U8 len_counts[gc_max_code_len_] = {};
  for (int i = 0; i < count; ++i) {
    if (lens[i]) {
      alphabet->cfls[lens[i]].codes[len_counts[lens[i]]] = i;
      len_counts[lens[i]]++;
    }
  }
  for (int i = 0; i < gc_max_code_len_; ++i) {
    if (len_counts[i]) {
      alphabet->min_len = i;
      break;
    }
  }
  for (U8 i = gc_max_code_len_ - 1; i > 0; --i) {
    if (len_counts[i]) {
      alphabet->max_len = i;
      break;
    }
  }

  int smallest_code = 0;
  for (U8 i = alphabet->min_len; i <= alphabet->max_len; ++i) {
    smallest_code = smallest_code;
    alphabet->cfls[i].min = smallest_code;
    alphabet->cfls[i].count = len_counts[i];
    smallest_code = (smallest_code + len_counts[i]) << 1;
  }

  for (int i = alphabet->min_len; i <= M_fast_bit_count; ++i) {
    for (int j = 0; j < alphabet->cfls[i].count; ++j) {
      U16 code = alphabet->cfls[i].codes[j];
      Symbol_t_ fast_code = {code, (U8)i};
      U16 c = alphabet->cfls[i].min + j;
      c = reverse16_(c);
      c = c >> (16 - i);
      int max_prefix = 1 << (M_fast_bit_count - i);
      for (int k = 0; k < max_prefix; ++k) {
        alphabet->fast_table[c | (k << i)] = fast_code;
      }
    }
  }
}

inline static int decode_(Bit_stream_t* bs, const Alphabet_t_* c_alphabet) {
  U16 v = bs->peek_lsb(M_fast_bit_count);
  Symbol_t_ code = c_alphabet->fast_table[v];
  if (code.len) {
    bs->skip(code.len);
    return code.val;
  }

  bs->skip(M_fast_bit_count);
  v = reverse16_(v) >> (16 - M_fast_bit_count);

  for (U8 i = M_fast_bit_count; i <= c_alphabet->max_len; ++i) {
    int delta_to_min = v - c_alphabet->cfls[i].min;
    if (delta_to_min < c_alphabet->cfls[i].count) {
      return c_alphabet->cfls[i].codes[delta_to_min];
    }
    v = v << 1 | bs->consume_msb(1);
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

inline static int paeth_(int a, int b, int c) {
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

inline U16 Bit_stream_t::peek_lsb(Sip bit_count) {
  M_check(bit_count <= 16);
  int remaining_bit_count = 32 - m_bit_index;
  if (bit_count <= remaining_bit_count) {
    return (m_cache_lsb >> m_bit_index) & ((1 << bit_count) - 1);
  }
  U16 rv = m_cache_lsb >> m_bit_index;
  bit_count -= remaining_bit_count;
  if (bit_count <= 8) {
    rv |= (m_data[m_byte_index + 4] & ((1 << bit_count) - 1)) << remaining_bit_count;
    return rv;
  }
  rv |= m_data[m_byte_index + 4] << remaining_bit_count;
  bit_count -= 8;
  rv |= (m_data[m_byte_index + 5] & ((1 << bit_count) - 1)) << (remaining_bit_count + 8);
  return rv;
}

inline U16 Bit_stream_t::consume_lsb(Sip bit_count) {
  U16 rv = peek_lsb(bit_count);
  skip(bit_count);
  return rv;
}

inline U16 Bit_stream_t::consume_msb(Sip bit_count) {
  U16 rv = consume_lsb(bit_count);
  rv = ((rv & 0xAAAA) >>  1) | ((rv & 0x5555) << 1);
  rv = ((rv & 0xCCCC) >>  2) | ((rv & 0x3333) << 2);
  rv = ((rv & 0xF0F0) >>  4) | ((rv & 0x0F0F) << 4);
  rv = ((rv & 0xFF00) >>  8) | ((rv & 0x00FF) << 8);
  rv = rv >> (16 - bit_count);
  return rv;
}

inline void Bit_stream_t::skip(Sip bit_count) {
  m_bit_index += bit_count;
  if (m_bit_index > 31) {
    m_bit_index = m_bit_index % 32;
    m_byte_index += 4;
    cache_();
  }
}

inline void Bit_stream_t::cache_() {
  m_cache_lsb = 0;
  // Little endian
  for (int i = 0; i < 4 && m_byte_index + i < m_len; ++i) {
    m_cache_lsb |= m_data[m_byte_index + i] << (i * 8);
  }
}

bool Png_loader_t::init(Allocator_t* allocator, const Path_t& path) {
  m_allocator = allocator;

  Linear_allocator_t<> temp_allocator("PNG_loader_temp_allocator");
  M_scope_exit(temp_allocator.destroy());
  Dynamic_array_t<U8> data = File_t::read_whole_file_as_text(&temp_allocator, path.m_path);
  M_check_log_return_val(!memcmp(&data[0], &gc_png_signature_[0], gc_png_sig_len_), false, "Invalid PNG signature");
  Dynamic_array_t<U8> idat_full(&temp_allocator);
  for (int i = gc_png_sig_len_; i < data.len();) {
    int data_len = M_bswap32_(*((int*)(&data[0] + i)));
    i += 4;
    const U8* chunk_it = &data[0] + i;
    M_unused(chunk_it);
    const int chunk_type = *((int*)(&data[0] + i));
    i += 4;
    U8* p = &data[0] + i;
    i += data_len;
    // U8* cRC = it;
    i += 4;
    switch (chunk_type) {
    case four_cc("IHDR"): {
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
        m_components_per_pixel = 1;
        break;
      case 2:
        // RGB
        m_components_per_pixel = 3;
        break;
      case 3:
        M_unimplemented();
        break;
      case 4:
        // Grey scale with alpha
        m_components_per_pixel = 2;
      case 6:
        // RGBA
        m_components_per_pixel = 4;
        break;
      default:
        M_logf_return_val(false, "Invalid color type");
      }
      M_check_log(m_bit_depth == 8 || m_bit_depth == 16, "No support for bit depth <8");
      if (m_bit_depth == 8) {
        switch(m_components_per_pixel) {
          case 1:
            m_format = e_format_r8_uint;
            break;
          case 3:
          case 4:
            m_format = e_format_r8g8b8a8_uint;
            break;
          default:
            M_unimplemented();
        }
      } else if (m_bit_depth == 16) {
        switch(m_components_per_pixel) {
          case 1:
            m_format = e_format_r16_uint;
            break;
          case 3:
          case 4:
            m_format = e_format_r16g16b16a16_uint;
            break;
          default:
            M_unimplemented();
        }
      } else {
        M_unimplemented();
      }
      m_bytes_per_pixel = m_bit_depth / 8 * m_components_per_pixel;
      idat_full.reserve((m_width + 1) * m_height * m_bytes_per_pixel);
      U8 compression_method = *p++;
      M_check_log_return_val(!compression_method, false, "Invalid compression method");
      U8 filter_method = *p++;
      M_check_log_return_val(!filter_method, false, "Invalid filter method");
      const U8 interlace_method = *p++;
      M_check_log_return_val(!interlace_method, false, "Invalid interlace method");
      break;
    }
    case four_cc("PLTE"): {
      break;
    }
    case four_cc("IDAT"): {
      idat_full.append_array(p, data_len);
      break;
    }
    case four_cc("IEND"): {
      Bit_stream_t bs(idat_full.m_p, idat_full.len());
      // 2 bytes of zlib header.
      U32 zlib_compress_method = bs.consume_lsb(4);
      M_check_log_return_val(zlib_compress_method == 8, false, "Invalid zlib compression method");
      U32 zlib_compress_info = bs.consume_lsb(4);
      M_unused(zlib_compress_info);
      M_check_log_return_val((idat_full[0] * 256 + idat_full[1]) % 31 == 0, false, "Invalid FCHECK bits");
      bs.consume_lsb(5);
      U8 fdict = bs.consume_lsb(1);
      M_unused(fdict);
      U8 flevel = bs.consume_lsb(2);
      M_unused(flevel);
      U8* deflated_data = (U8*)temp_allocator.alloc((m_width + 1) * m_height * m_bytes_per_pixel);
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
          Alphabet_t_ code_len_alphabet = {};
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
          Alphabet_t_ lit_or_len_alphabet = {};
          build_alphabet_(lit_and_dist_lens, hlit, &lit_or_len_alphabet);
          Alphabet_t_ dist_alphabet = {};
          build_alphabet_(lit_and_dist_lens + hlit, hdist, &dist_alphabet);
          for (;;) {
            int lit_or_len_code = decode_(&bs, &lit_or_len_alphabet);
            if (lit_or_len_code < 256) {
              *deflated_p++ = lit_or_len_code;
              continue;
            }
            if (lit_or_len_code == 256) {
              break;
            }
            decode_len_and_dist_(&deflated_p, lit_or_len_code, &bs, &dist_alphabet);
          }
        }
        if (bfinal) {
          break;
        }
      }
      m_data = (U8*)m_allocator->alloc(m_width * m_height * m_bytes_per_pixel);
      int bytes_per_deflated_row = m_bytes_per_pixel * m_width + 1;
      int bytes_per_data_row = m_bytes_per_pixel * m_width;
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
          for (int j = 0; j < m_bytes_per_pixel; ++j) {
            m_data[data_offset + j] = deflated_data[deflated_offset + j];
          }
          for (int j = m_bytes_per_pixel; j < bytes_per_data_row; ++j) {
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
            for (int j = 0; j < m_bytes_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j];
            }
            for (int j = m_bytes_per_pixel; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *a++ / 2;
            }
          } else {
            for (int j = 0; j < m_bytes_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + *b++ / 2;
            }
            for (int j = m_bytes_per_pixel; j < bytes_per_data_row; ++j) {
              int p_i = j - m_bytes_per_pixel;
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + (a[p_i] + b[p_i]) / 2;
            }
          }
        } break;
        case 4: {
          if (!r) {
            for (int j = 0; j < bytes_per_data_row; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j];
            }
          } else {
            for (int j = 0; j < m_bytes_per_pixel; ++j) {
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + paeth_(0, *b++, 0);
            }
            for (int j = m_bytes_per_pixel; j < bytes_per_data_row; ++j) {
              int p_i = j - m_bytes_per_pixel;
              m_data[data_offset + j] = deflated_data[deflated_offset + j] + paeth_(a[p_i], b[p_i], c[p_i]);
            }
          }
        } break;
        default:
          M_logf_return_val(false, "Invalid filter method");
        }
      }
      if (m_bit_depth == 16) {
        for (int j = 0; j < m_bytes_per_pixel * m_width * m_height; j = j + 2) {
          swap(&m_data[j], &m_data[j+1]);
        }
      }
      if (m_components_per_pixel == 3) {
        U8* temp_data = (U8*)temp_allocator.alloc(m_width * m_height * m_bytes_per_pixel);
        memcpy(temp_data, m_data, m_width * m_height * m_bytes_per_pixel);
        U8 new_components_per_pixel = 4;
        U8 new_bytes_per_pixel = m_bit_depth * new_components_per_pixel / 8;
        m_data = (U8*)allocator->realloc(m_data, m_width * m_height * new_bytes_per_pixel);
        for (int j = 0; j < m_width * m_height; ++j) {
          memcpy(m_data + j * new_bytes_per_pixel, temp_data + j * m_bytes_per_pixel, m_bytes_per_pixel);
          // Set alpha to max
          for (int k = m_bytes_per_pixel; k < new_bytes_per_pixel; ++k) {
            m_data[j*new_bytes_per_pixel + k] = 255;
          }
        }
        m_components_per_pixel = new_components_per_pixel;
        m_bytes_per_pixel = new_bytes_per_pixel;
      }
      break;
    }
    }
  }
  return true;
}

void Png_loader_t::destroy() {
  m_allocator->free(m_data);
}
