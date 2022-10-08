//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/ttf.h"

#include "core/file.h"
#include "core/hash_table.inl"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/string.inl"
#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb/stb_truetype.h"

#include <stdio.h>

struct Table_t_ {
  U32 checksum;
  U32 offset;
  U32 length;
};

struct Head_table_t_ {
  U32 version;
  U32 font_revision;
  U32 check_sum_adjustment;
  U32 magic_number;
  U16 flags;
  U16 units_per_em;
  S64 created;
  S64 modified;
  U16 x_min;
  U16 y_min;
  U16 x_max;
  U16 y_max;
  U16 mac_style;
  U16 lowest_rec_ppem;
  S16 font_direction_hint;
  S16 index_to_loc_format;
  S16 glyph_data_format;
};

template <typename T>
static T read_be_(const U8* p) {
  T rv = 0;
  constexpr int byte_count = sizeof(T);
  for (int i = 0; i < byte_count; ++i) {
    rv |= p[i] << ((byte_count - i - 1) * 8);
  }
  return rv;
}

template <typename T>
static T consume_be_(const U8** p) {
  T rv = read_be_<T>(*p);
  *p += sizeof(T);
  return rv;
}

Cstring_t read_tag_(const U8** p) {
  Cstring_t rv((const char*)(*p), 4);
  *p += 4;
  return rv;
}

Ttf_loader_t::Ttf_loader_t(Allocator_t* allocator) : m_points(allocator) {
}

bool Ttf_loader_t::init(const Path_t& path) {
  Linear_allocator_t<> temp_allocator("ttf_allocator");
  Dynamic_array_t<U8> ttf = File_t::read_whole_file_as_binary(&temp_allocator, path.m_path);

  stbtt_fontinfo font;
  stbtt_InitFont(&font, ttf.m_p, stbtt_GetFontOffsetForIndex(ttf.m_p, 0));
  stbtt_vertex *vertices;
  auto a1 = stbtt__GetGlyphShapeTT(&font, 'a', &vertices);

  const U8* cp = ttf.m_p;
  const U8* p = ttf.m_p;
  U32 scaler_type = consume_be_<U32>(&p);
  M_check(scaler_type == 0x74727565 || scaler_type == 0x00010000);
  // offset subtable
  U16	num_tables = consume_be_<U16>(&p);
  U16	search_range = consume_be_<U16>(&p);
  U16	entry_selector = consume_be_<U16>(&p);
  U16	range_shift = consume_be_<U16>(&p);
  Hash_map_t<Cstring_t, Table_t_> tables(&temp_allocator);
  tables.reserve(num_tables);
  for (int i = 0; i < num_tables; ++i) {
    Cstring_t tag = read_tag_(&p);
    Table_t_ table = {};
    table.checksum = consume_be_<U32>(&p);
    table.offset = consume_be_<U32>(&p);
    table.length = consume_be_<U32>(&p);
    tables[tag] = table;
  }


  S16 index_to_loc_format = read_be_<S16>(cp + tables["head"].offset + 50);

  char a = 'a';
  U32 glyf_offset = 0;
  if (index_to_loc_format == 0) {
    glyf_offset = read_be_<U16>(cp + tables["loca"].offset + a * sizeof(U16));
  } else {
    glyf_offset = read_be_<U32>(cp + tables["loca"].offset + a * sizeof(U32));
  }
  const U8* glyf = cp + tables["glyf"].offset + glyf_offset;
  S16 number_of_contours = read_be_<S16>(glyf);
  const U8* end_pts_of_contours = glyf + 10;
  p = end_pts_of_contours + number_of_contours * sizeof(U16);
  U16 point_count = read_be_<U16>(end_pts_of_contours + (number_of_contours -1)*sizeof(U16)) + 1;
  U16 instruction_length = consume_be_<U16>(&p);
  const U8* instructions = p;
  const U8* flags = instructions + instruction_length;
  m_points.resize(point_count);
  for (int i = 0; i < point_count; ++i) {
    U8 flag = *(flags++);
    m_points[i].flag = flag;
    if (flag & 8) {
      U8 repeat_count = *(flags++);
      for (int j = 1; j <= repeat_count; ++j) {
        m_points[i + j].flag = flag;
      }
      i += repeat_count;
    }
  }

  const U8* x_coords = flags;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = m_points[i].flag;
    if (flag & 2) {
      if (flag & 0x10) {
        m_points[i].x = *x_coords++;
      } else {
        m_points[i].x = -*x_coords++;
      }
    } else {
      if (flag & 0x10) {
        m_points[i].x = m_points[i - 1].x;
      } else {
        m_points[i].x = consume_be_<S16>(&x_coords);
      }
    }
  }

  const U8* y_coords = x_coords;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = m_points[i].flag;
    if (flag & 4) {
      if (flag & 0x20) {
        m_points[i].y = *y_coords++;
      } else {
        m_points[i].y = -*y_coords++;
      }
    } else {
      if (flag & 0x20) {
        m_points[i].y = m_points[i - 1].y;
      } else {
        m_points[i].y = consume_be_<S16>(&y_coords);
      }
    }
  }
  return true;
}
