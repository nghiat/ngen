//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/ttf.h"

#include "core/allocator.h"
#include "core/file.h"
#include "core/fixed_array.h"
#include "core/hash_table.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/math/vec2.h"
#include "core/string.h"

#include <stdio.h>

#include <algorithm>

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

struct Line_t_ {
  V2_t p[2];
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

Ttf_loader_t::Ttf_loader_t(Allocator_t* allocator) : m_allocator(allocator) {
}

bool Ttf_loader_t::init(const Path_t& path) {
  Linear_allocator_t<> temp_allocator("ttf_allocator");
  M_scope_exit(temp_allocator.destroy());
  Dynamic_array_t<U8> ttf = File_t::read_whole_file_as_binary(&temp_allocator, path.m_path);

  const U8* cp = ttf.m_p;
  const U8* p = ttf.m_p;
  U32 scaler_type = consume_be_<U32>(&p);
  M_check(scaler_type == 0x74727565 || scaler_type == 0x00010000);
  U16 num_tables = consume_be_<U16>(&p);
  p += 6;
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

  auto copy_table = [this, &cp, &tables](const char* name) -> U8* {
    Table_t_ table = tables[name];
    auto rv = (U8*)m_allocator->alloc(table.length);
    memcpy(rv, cp + table.offset, table.length);
    return rv;
  };

  m_head_table = copy_table("head");
  m_cmap_table = copy_table("cmap");
  m_loca_table = copy_table("loca");
  m_glyf_table = copy_table("glyf");
  m_hhea_table = copy_table("hhea");
  m_hmtx_table = copy_table("hmtx");

  return true;
}

void Ttf_loader_t::get_glyph(Glyph_t* glyph, const char c, int height_in_pixel) {
  Linear_allocator_t<> temp_allocator("get_glyph_allocator");
  M_scope_exit(temp_allocator.destroy());
  int glyph_id = 0;
  const U8* cmap = m_cmap_table;
  cmap += 2;
  U16 number_subtables = consume_be_<U16>(&cmap);
  for (int i = 0; i < number_subtables; ++i) {
    cmap += 4;
    U32 subtable_offset = consume_be_<U32>(&cmap);
    const U8* table = m_cmap_table + subtable_offset;
    U16 format = consume_be_<U16>(&table);
    M_check(format == 4);
    U16 length = consume_be_<U16>(&table);
    const U8* end_table = table + length;
    table += 2;
    U16 seg_count_x2 = consume_be_<U16>(&table);
    U16 seg_count = seg_count_x2 / 2;
    table += 6;
    Dynamic_array_t<U16> end_codes(&temp_allocator);
    end_codes.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      end_codes[j] = consume_be_<U16>(&table);
    }
    M_check(end_codes.last() == 0xffff);
    table += 2;
    Dynamic_array_t<U16> start_codes(&temp_allocator);
    start_codes.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      start_codes[j] = consume_be_<U16>(&table);
    }
    Dynamic_array_t<S16> deltas(&temp_allocator);
    deltas.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      deltas[j] = consume_be_<U16>(&table);
    }
    const U8* id_range_offsets_p = table;
    Dynamic_array_t<U16> id_range_offsets(&temp_allocator);
    id_range_offsets.resize(seg_count);
    for (int j = 0; j < seg_count; ++j) {
      id_range_offsets[j] = consume_be_<U16>(&table);
    }
    Dynamic_array_t<U16> glyph_ids(&temp_allocator);
    while (table != end_table) {
      glyph_ids.append(consume_be_<U16>(&table));
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

  S16 index_to_loc_format = read_be_<S16>(m_head_table + 50);
  U32 glyf_offset = 0;
  if (index_to_loc_format == 0) {
    glyf_offset = read_be_<U16>(m_loca_table + glyph_id * sizeof(U16));
  } else {
    glyf_offset = read_be_<U32>(m_loca_table + glyph_id * sizeof(U32));
  }
  const U8* glyf = m_glyf_table + glyf_offset;
  S16 number_of_contours = consume_be_<S16>(&glyf);
  M_check_log(number_of_contours >= 0, "Only simple glyph for now");
  S16 x_min = consume_be_<S16>(&glyf);
  S16 y_min = consume_be_<S16>(&glyf);
  S16 x_max = consume_be_<S16>(&glyf);
  S16 y_max = consume_be_<S16>(&glyf);
  Dynamic_array_t<U16> end_pts_of_contours(&temp_allocator);
  end_pts_of_contours.resize(number_of_contours);
  for (int i = 0; i < number_of_contours; ++i) {
    end_pts_of_contours[i] = consume_be_<U16>(&glyf);
  }
  U16 point_count = end_pts_of_contours.last() + 1;
  U16 instruction_length = consume_be_<U16>(&glyf);
  glyf += instruction_length;
  Dynamic_array_t<Point_t_> ttf_points(&temp_allocator);
  ttf_points.resize(point_count);
  for (int i = 0; i < point_count; ++i) {
    U8 flag = *(glyf++);
    ttf_points[i].flag = flag;
    if (flag & 8) {
      U8 repeat_count = *(glyf++);
      for (int j = 1; j <= repeat_count; ++j) {
        ttf_points[i + j].flag = flag;
      }
      i += repeat_count;
    }
  }

  S16 x = 0;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = ttf_points[i].flag;
    if (flag & 2) {
      if (flag & 0x10) {
        x += *glyf++;
      } else {
        x -= *glyf++;
      }
    } else {
      if (flag & 0x10) {
        x = ttf_points[i - 1].x;
      } else {
        x += consume_be_<S16>(&glyf);
      }
    }
    ttf_points[i].x = x;
  }

  S16 y = 0;
  for (int i = 0; i < point_count; ++i) {
    U8 flag = ttf_points[i].flag;
    if (flag & 4) {
      if (flag & 0x20) {
        y += *glyf++;
      } else {
        y -= *glyf++;
      }
    } else {
      if (flag & 0x20) {
        y = ttf_points[i - 1].y;
      } else {
        y += consume_be_<S16>(&glyf);
      }
    }
    ttf_points[i].y = y;
  }
  int from = 0;
  Dynamic_array_t<Line_t_> lines(&temp_allocator);
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

    const Point_t_* pp = reconstructed_points.m_p;
    for (int j = 0; j < reconstructed_points.len() - 1; ++j) {
      if (pp[j+1].flag & M_on_the_curve_flag_) {
        Line_t_ l;
        l.p[0] = V2_t{pp[j].x, pp[j].y};
        l.p[1] = V2_t{pp[j+1].x, pp[j+1].y};
        lines.append(l);
      } else {
        V2_t p0 = V2_t{pp[j].x, pp[j].y};
        V2_t p1 = V2_t{pp[j+1].x, pp[j+1].y};
        V2_t p2 = V2_t{pp[j+2].x, pp[j+2].y};
        int n = 10;
        for (int k = 0; k < n; ++k) {
          Line_t_ l;
          float t = k*1.f/n;
          l.p[0] = p0*(1-t)*(1-t) + p1*2*t*(1-t) + p2*t*t;
          t = (k+1)*1.f/n;
          l.p[1] = p0*(1-t)*(1-t) + p1*2*t*(1-t) + p2*t*t;
          lines.append(l);
        }
        j += 1; // will be incremented once more
      }
    }

    from = to + 1;
  }

  U16 units_per_em = read_be_<U16>(m_head_table + 18);
  S16 ascender = read_be_<S16>(m_hhea_table + 4);
  S16 descender = read_be_<S16>(m_hhea_table + 6);
  float scale = height_in_pixel * 1.f / (ascender - descender);
  for (auto& line : lines) {
    line.p[0].x -= x_min;
    line.p[0].x *= scale;
    line.p[0].y -= y_min;
    line.p[0].y *= scale;
    line.p[1].x -= x_min;
    line.p[1].x *= scale;
    line.p[1].y -= y_min;
    line.p[1].y *= scale;
  }

  U16 number_of_h_metrics = read_be_<U16>(m_hhea_table + 34);
  S16 lsb;
  U16 aw;
  if (glyph_id < number_of_h_metrics) {
    aw = read_be_<U16>(m_hmtx_table + 4*glyph_id);
    lsb = read_be_<S16>(m_hmtx_table + 4*glyph_id + 2);
  } else {
    aw = read_be_<U16>(m_hmtx_table + 4*(number_of_h_metrics - 1));
    lsb = read_be_<S16>(m_hmtx_table + 4*number_of_h_metrics + 2*(glyph_id - number_of_h_metrics));
  }

  glyph->texture = (U8*)m_allocator->alloc_zero(height_in_pixel*height_in_pixel);
  glyph->offset_x = lsb*scale;
  glyph->offset_y = y_min*scale;
  glyph->advance = aw*scale;
  glyph->width = (U16)(x_max - y_min + height_in_pixel - 1)*scale; // round up
  glyph->height = (U16)(y_max - y_min + height_in_pixel - 1)*scale; // round up
  std::sort(lines.begin(), lines.end(), [](const Line_t_ l1, const Line_t_ l2) {
    float min_y1 = std::min(l1.p[0].y, l1.p[1].y);
    float min_y2 = std::min(l2.p[0].y, l2.p[1].y);
    return min_y1 < min_y2;
  });

  for (int y = 0; y < height_in_pixel; ++y) {
    struct Intersect_t_ {
      float x;
      Line_t_ l;
    };
    Fixed_array_t<Intersect_t_, 20> intersects;
    for (int i = 0; i < lines.len(); ++i) {
      const Line_t_ l = lines[i];
      float y0 = l.p[0].y;
      float y1 = l.p[1].y;
      if (y0 == y1) {
        if (y0 != y) {
          continue;
        }
        // handle paralell
        M_unimplemented();
      } else {
        float t = (y - y0)/(y1 - y0);
        if (t >= 0.0f && t <= 1.0f) {
          float x_t = l.p[0].x + t*(l.p[1].x - l.p[0].x);
          intersects.append(Intersect_t_{x_t, l});
        }
      }
    }
    std::sort(intersects.begin(), intersects.end(), [](const Intersect_t_& i1, const Intersect_t_& i2) {
        return i1.x < i2.x;
    });
    for (int i = 0; i < intersects.len() - 1; ++i) {
      // When two lines cross the scanline at the same point, only keep the point that is equals to std::min(p0, p1)
      if (intersects[i].x == intersects[i+1].x) {
        glyph->texture[(height_in_pixel - y)*height_in_pixel + (int)intersects[i].x] = 255;
        if (y != std::min(intersects[i].l.p[0].y, intersects[i].l.p[1].y)) {
          intersects.remove_at(i);
          --i;
        }
        if (y != std::min(intersects[i+1].l.p[0].y, intersects[i+1].l.p[1].y)) {
          intersects.remove_at(i+1);
        }
      }
    }
    for (int i = 0; i < intersects.len()/2; ++i) {
      for (int x = intersects[2*i].x; x <= intersects[2*i + 1].x; ++x) {
        glyph->texture[(height_in_pixel - y)*height_in_pixel + x] = 255;
      }

    }
  }
}
