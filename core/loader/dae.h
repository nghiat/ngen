//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/fixed_array.h"
#include "core/math/mat4.h"
#include "core/math/vec3.h"
#include "core/math/vec4.h"
#include "core/path.h"

class Allocator_t;

struct Joint_t {
  Joint_t(Allocator_t* allocator) : children(allocator) {}
  M4_t default_mat;
  int mat_idx = 0;
  Dynamic_array_t<Joint_t*> children;
};

struct Animation_t {
  Animation_t(Allocator_t* allocator) : times(allocator), matrices(allocator) {}

  float duration;
  Joint_t* joint;
  Dynamic_array_t<float> times;
  Dynamic_array_t<M4_t> matrices;
};

void anim_destroy(Animation_t* animation);
M4_t anim_get_at(const Animation_t* animation, float time);

struct Vertex_t {
  V4_t position;
  V3_t normal;
  U32 joint_idx[16] = {};
  float weights[16] = {};
};

class Dae_loader_t {
public:
  Dae_loader_t(Allocator_t* allocator);
  bool init(const Path_t& path);
  void destroy();
  void update_joint_matrices_at(float time_s);

  Dynamic_array_t<Vertex_t> m_vertices;
  Dynamic_array_t<Animation_t> m_animations;
  Dynamic_array_t<M4_t> m_joint_matrices;
  Dynamic_array_t<M4_t> m_inv_bind_matrices;
  Joint_t m_root_joint;
};
