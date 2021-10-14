//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
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

static void skip_space(char** p) {
  while (**p == ' ')
    ++(*p);
}

static void skip_till(char** p, char c) {
  while(**p != c)
    ++(*p);
}

static void string_to_vec(char** p, int len, float* v) {
  for (int j = 0; j < len; ++j) {
    skip_till(p, ' ');
    skip_space(p);
    v[j] = atof(*p);
  }
}

static inline int parse_index(int index, int len) {
  if (!index)
    return -1;
  if (index > 0)
    return index - 1;
  return index + len;
}

bool OBJLoader::obj_init(ngAllocator* allocator, const OSChar* path) {
  LinearAllocator<> temp_allocator("OBJLoader_allocator");
  temp_allocator.la_init();
  SCOPE_EXIT(temp_allocator.al_destroy());

  int vs_count = 0;
  int uvs_count = 0;
  int ns_count = 0;
  int elems_count = 0;
  DynamicArray<U8> f = ngFile::f_read_whole_file_as_text(&temp_allocator, path);
  char* s = (char*)&f[0];
  char* e = (char*)&f[0] + f.da_len();
  for (;;) {
    while(isspace(*s)) {
      ++s;
    }
    if(s[0] == 'v' && s[1] == ' ') {
      ++vs_count;
    }
    if(s[0] == 'v' && s[1] == 't') {
      ++uvs_count;
    }
    if(s[0] == 'v' && s[1] == 'n') {
      ++ns_count;
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
  DynamicArray<V4> vs;
  DynamicArray<V2> uvs;
  DynamicArray<V4> ns;
  vs.da_init(&temp_allocator);
  uvs.da_init(&temp_allocator);
  ns.da_init(&temp_allocator);
  vs.da_reserve(vs_count);
  uvs.da_reserve(vs_count);
  ns.da_reserve(vs_count);

  m_vertices.da_init(allocator);
  m_uvs.da_init(allocator);
  m_normals.da_init(allocator);
  vs.da_reserve(elems_count);
  if (uvs_count)
    uvs.da_reserve(elems_count);
  if (ns_count)
    ns.da_reserve(elems_count);

  s = (char*)&f[0];
  while (s != e) {
    while (isspace(*s)) {
      ++s;
    }
    if (s[0] == 'v' && s[1] == ' ') {
      V4 v;
      string_to_vec(&s, 3, &v.x);
      v.w = 1.0f;
      vs.da_append(v);
    } else if (s[0] == 'v' && s[1] == 't') {
      V2 v;
      string_to_vec(&s, 2, &v.x);
      uvs.da_append(v);
    } else if (s[0] == 'v' && s[1] == 'n') {
      V3 v;
      string_to_vec(&s, 3, &v.x);
      v = v3_normalize(v);
      ns.da_append({v.x, v.y, v.z, 0.0f});
    } else if (s[0] == 'f' && s[1] == ' ') {
      for (int j = 0; j < 3; ++j) {
        int index;
        skip_till(&s, ' ');
        index = parse_index(atoi(++s), vs_count);
        m_vertices.da_append(vs[index]);
        skip_till(&s, '/');
        index = parse_index(atoi(++s), uvs_count);
        if (index != -1) {
          m_uvs.da_append(uvs[index]);
        }
        skip_till(&s, '/');
        index = parse_index(atoi(++s), ns_count);
        if (index != -1) {
          m_normals.da_append(ns[index]);
        }
      }
    }
    while (*s != '\n' && s != e) {
      ++s;
    }
  }

  return true;
}

void OBJLoader::obj_destroy() {
  m_normals.da_destroy();
  m_uvs.da_destroy();
  m_vertices.da_destroy();
}
