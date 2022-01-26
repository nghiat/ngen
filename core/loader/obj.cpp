//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/obj.h"

#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/linear_allocator.h"
#include "core/log.h"
#include "core/math/vec3.h"
#include "core/utils.h"

#include <ctype.h>
#include <stdlib.h>

static void skip_space_(char** p) {
  while (**p == ' ')
    ++(*p);
}

static void skip_till_(char** p, char c) {
  while(**p != c)
    ++(*p);
}

static void string_to_vec_(char** p, int len, float* v) {
  for (int j = 0; j < len; ++j) {
    skip_till_(p, ' ');
    skip_space_(p);
    v[j] = atof(*p);
  }
}

static inline int parse_index_(int index, int len) {
  if (!index)
    return -1;
  if (index > 0)
    return index - 1;
  return index + len;
}

bool Obj_loader_t::init(Allocator_t* allocator, const Os_char* path) {
  Linear_allocator_t<> temp_allocator("Obj_loader_allocator");
  temp_allocator.init();
  M_scope_exit(temp_allocator.destroy());

  int v_count = 0;
  int uv_count = 0;
  int n_count = 0;
  int elems_count = 0;
  Dynamic_array_t<U8> f = File_t::read_whole_file_as_text(&temp_allocator, path);
  char* s = (char*)&f[0];
  char* e = (char*)&f[0] + f.len();
  for (;;) {
    while(isspace(*s)) {
      ++s;
    }
    if(s[0] == 'v' && s[1] == ' ') {
      ++v_count;
    }
    if(s[0] == 'v' && s[1] == 't') {
      ++uv_count;
    }
    if(s[0] == 'v' && s[1] == 'n') {
      ++n_count;
    }
    if(s[0] == 'f' && s[1] == ' ') {
      ++elems_count;
    }
    while (*s != '\n' && s != e) {
      ++s;
    }
    if (s == e) {
      break;
    }
  }
  Dynamic_array_t<V4_t> vs;
  Dynamic_array_t<V2_t> uvs;
  Dynamic_array_t<V4_t> ns;
  vs.init(&temp_allocator);
  uvs.init(&temp_allocator);
  ns.init(&temp_allocator);
  vs.reserve(v_count);
  uvs.reserve(v_count);
  ns.reserve(v_count);

  m_vertices.init(allocator);
  m_uvs.init(allocator);
  m_normals.init(allocator);
  vs.reserve(elems_count);
  if (uv_count)
    uvs.reserve(elems_count);
  if (n_count)
    ns.reserve(elems_count);

  s = (char*)&f[0];
  while (s != e) {
    while (isspace(*s)) {
      ++s;
    }
    if (s[0] == 'v' && s[1] == ' ') {
      V4_t v;
      string_to_vec_(&s, 3, &v.x);
      v.w = 1.0f;
      vs.append(v);
    } else if (s[0] == 'v' && s[1] == 't') {
      V2_t v;
      string_to_vec_(&s, 2, &v.x);
      uvs.append(v);
    } else if (s[0] == 'v' && s[1] == 'n') {
      V3_t v;
      string_to_vec_(&s, 3, &v.x);
      v = v3_normalize(v);
      ns.append({v.x, v.y, v.z, 0.0f});
    } else if (s[0] == 'f' && s[1] == ' ') {
      for (int j = 0; j < 3; ++j) {
        int index;
        skip_till_(&s, ' ');
        index = parse_index_(atoi(++s), v_count);
        m_vertices.append(vs[index]);
        skip_till_(&s, '/');
        index = parse_index_(atoi(++s), uv_count);
        if (index != -1) {
          m_uvs.append(uvs[index]);
        }
        skip_till_(&s, '/');
        index = parse_index_(atoi(++s), n_count);
        if (index != -1) {
          m_normals.append(ns[index]);
        }
      }
    }
    while (*s != '\n' && s != e) {
      ++s;
    }
  }

  return true;
}

void Obj_loader_t::destroy() {
  m_normals.destroy();
  m_uvs.destroy();
  m_vertices.destroy();
}
