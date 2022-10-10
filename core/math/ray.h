//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/plane.h"
#include "core/math/sphere.h"
#include "core/math/triangle.h"
#include "core/math/vec3.h"

struct Ray_t {
  V3_t origin;
  V3_t dir;
};

V3_t ray_at(const Ray_t& r, F32 t);
bool ray_hit_plane(F32* out_t, const Ray_t& r, const Plane_t& p);
bool ray_hit_sphere(F32* out_t, const Ray_t& r, const Sphere_t& s);
bool ray_hit_triangle(const Ray_t& r, const Triangle_t& t);

#include "core/math/ray.inl"
