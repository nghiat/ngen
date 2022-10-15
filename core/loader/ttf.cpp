//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/ttf.h"

#include "core/file.h"
#include "core/hash_table.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/math/vec2.h"
#include "core/string.h"

#include <stdio.h>

#define M_on_the_curve_flag_ 1

struct Table_t_ {
  U32 checksum;
  U32 offset;
  U32 length;
};

struct Point_t_ {
  float x = 0;
  float y = 0;
  U8 flag = 0;
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

  char c = '8';
  const U8* cmap = cp + tables["cmap"].offset;
  p = cmap;
  U16 version = consume_be_<U16>(&p);
  int glyph_id = 0;
  U16 number_subtables = consume_be_<U16>(&p);
  for (int i = 0; i < number_subtables; ++i) {
    U16 platform_id = consume_be_<U16>(&p);
    U16 encoding_id = consume_be_<U16>(&p);
    U32 subtable_offset = consume_be_<U32>(&p);

    const U8* table = cmap + subtable_offset;
    const U8* p2 = table;
    U16 format = consume_be_<U16>(&p2);
    M_check(format == 4);
    U16 length = consume_be_<U16>(&p2);
    const U8* end_table = table + length;
    U16 language = consume_be_<U16>(&p2);
    U16 seg_count_x2 = consume_be_<U16>(&p2);
    U16 seg_count = seg_count_x2 / 2;
    U16 table_search_range = consume_be_<U16>(&p2);
    U16 table_entry_selector = consume_be_<U16>(&p2);
    U16 table_range_shift = consume_be_<U16>(&p2);
    Dynamic_array_t<U16> end_codes(&temp_allocator);
    end_codes.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      end_codes[j] = consume_be_<U16>(&p2);
    }
    M_check(end_codes.last() == 0xffff);
    U16 reserved_pad = consume_be_<U16>(&p2);
    M_check(reserved_pad == 0);
    Dynamic_array_t<U16> start_codes(&temp_allocator);
    start_codes.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      start_codes[j] = consume_be_<U16>(&p2);
    }
    Dynamic_array_t<S16> deltas(&temp_allocator);
    deltas.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      deltas[j] = consume_be_<S16>(&p2);
    }
    const U8* id_range_offsets_p = p2;
    Dynamic_array_t<U16> id_range_offsets(&temp_allocator);
    id_range_offsets.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      id_range_offsets[j] = consume_be_<U16>(&p2);
    }
    Dynamic_array_t<U16> glyph_ids(&temp_allocator);
    while (p2 != end_table) {
      glyph_ids.append(consume_be_<U16>(&p2));
    }
    for (int j = 0; j < end_codes.len(); ++j) {
      if (c <= end_codes[j] && c >= start_codes[j]) {
        if (id_range_offsets[j] == 0) {
          glyph_id = c + deltas[j];
        } else {
          glyph_id = read_be_<U16>(id_range_offsets_p + j*sizeof(U16) + id_range_offsets[j]/2 + (c - start_codes[j]));
        }
        break;
      }
    }
    if (glyph_id) {
      break;
    }
  }

  S16 index_to_loc_format = read_be_<S16>(cp + tables["head"].offset + 50);
  U32 glyf_offset = 0;
  if (index_to_loc_format == 0) {
    glyf_offset = read_be_<U16>(cp + tables["loca"].offset + glyph_id * sizeof(U16));
  } else {
    glyf_offset = read_be_<U32>(cp + tables["loca"].offset + glyph_id * sizeof(U32));
  }
  const U8* glyf = cp + tables["glyf"].offset + glyf_offset;
  p = glyf;
  S16 number_of_contours = consume_be_<S16>(&p);
  M_check_log(number_of_contours >= 0, "Only simple glyph for now");
  S16 x_min = consume_be_<S16>(&p);
  S16 y_min = consume_be_<S16>(&p);
  S16 x_max = consume_be_<S16>(&p);
  S16 y_max = consume_be_<S16>(&p);
  Dynamic_array_t<U16> end_pts_of_contours(&temp_allocator);
  end_pts_of_contours.resize(number_of_contours);
  for (int i = 0; i < number_of_contours; ++i) {
    end_pts_of_contours[i] = consume_be_<U16>(&p);
  }
  U16 point_count = end_pts_of_contours.last() + 1;
  U16 instruction_length = consume_be_<U16>(&p);
  const U8* instructions = p;
  const U8* flags = instructions + instruction_length;
  Dynamic_array_t<Point_t_> ttf_points(&temp_allocator);
  ttf_points.resize(point_count);
  for (int i = 0; i < point_count; ++i) {
    U8 flag = *(flags++);
    ttf_points[i].flag = flag;
    if (flag & 8) {
      U8 repeat_count = *(flags++);
      for (int j = 1; j <= repeat_count; ++j) {
        ttf_points[i + j].flag = flag;
      }
      i += repeat_count;
    }
  }

  const U8* x_coords = flags;
  S16 x = 0;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = ttf_points[i].flag;
    if (flag & 2) {
      if (flag & 0x10) {
        x += *x_coords++;
      } else {
        x -= *x_coords++;
      }
    } else {
      if (flag & 0x10) {
        x = ttf_points[i - 1].x;
      } else {
        x += consume_be_<S16>(&x_coords);
      }
    }
    ttf_points[i].x = x;
  }

  const U8* y_coords = x_coords;
  S16 y = 0;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = ttf_points[i].flag;
    if (flag & 4) {
      if (flag & 0x20) {
        y += *y_coords++;
      } else {
        y -= *y_coords++;
      }
    } else {
      if (flag & 0x20) {
        y = ttf_points[i - 1].y;
      } else {
        y += consume_be_<S16>(&y_coords);
      }
    }
    ttf_points[i].y = y;
  }
  int from = 0;
  for (int i = 0; i < end_pts_of_contours.len(); ++i) {
    int to = end_pts_of_contours[i];
    Dynamic_array_t<Point_t_> reconstructed_points(&temp_allocator);
    M_scope_exit(reconstructed_points.destroy());
    for (int j = from; j < to; ++j) {
      reconstructed_points.append(ttf_points[j]);
      if (!(ttf_points[j].flag & M_on_the_curve_flag_) && !(ttf_points[j + 1].flag & M_on_the_curve_flag_)) {
        Point_t_ mid_point;
        mid_point.flag |= M_on_the_curve_flag_;
        mid_point.x = (ttf_points[j].x + ttf_points[j+1].x)/2.f;
        mid_point.y = (ttf_points[j].y + ttf_points[j+1].y)/2.f;
        reconstructed_points.append(mid_point);
      }
    }
    reconstructed_points.append(ttf_points[to]);
    if (ttf_points[from].flag & M_on_the_curve_flag_) {
      reconstructed_points.append(ttf_points[from]);
    } else {
      Point_t_ mid_point;
      mid_point.flag |= M_on_the_curve_flag_;
      mid_point.x = (ttf_points[to].x + ttf_points[from].x)/2.f;
      mid_point.y = (ttf_points[to].y + ttf_points[from].y)/2.f;
      reconstructed_points.append(mid_point);
    }

    for (int j = 0; j < reconstructed_points.len(); ++j) {
      if (reconstructed_points[j].flag & M_on_the_curve_flag_) {
        M_logi("1");
      } else {
        M_logi("0");
      }
    }

    const Point_t_* pp = reconstructed_points.m_p;
    for (int j = 0; j < reconstructed_points.len() - 1; ++j) {
      if (pp[j+1].flag & M_on_the_curve_flag_) {
        m_points.append((V2_t){pp[j].x, pp[j].y});
        m_points.append((V2_t){pp[j+1].x, pp[j+1].y});
      } else {
        V2_t p0 = (V2_t){pp[j].x, pp[j].y};
        V2_t p1 = (V2_t){pp[j+1].x, pp[j+1].y};
        V2_t p2 = (V2_t){pp[j+2].x, pp[j+2].y};
        int n = 10;
        for (int k = 0; k < n; ++k) {
          float t = k*1.f/n;
          V2_t bezier_p = p0*(1-t)*(1-t) + p1*2*t*(1-t) + p2*t*t;
          m_points.append(bezier_p);
          t = (k+1)*1.f/n;
          bezier_p = p0*(1-t)*(1-t) + p1*2*t*(1-t) + p2*t*t;
          m_points.append(bezier_p);
        }
        j += 1; // will be incremented once more
      }
    }

    from = to + 1;
  }

  return true;
}
