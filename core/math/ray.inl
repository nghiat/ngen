//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/ray.h"

#include <math.h>

V3_t ray_at(Ray_t r, F32 t) {
  return r.origin + r.dir * t;
}

bool ray_hit_plane(F32* out_t, Ray_t r, Plane_t p) {
  F32 denominator = v3_dot(r.dir, p.normal);
  if (!float_equal_0(denominator)) {
    // Solve vec3_dot((o + td - p) , n) = 0;
    F32 t = v3_dot(p.p0 - r.origin, p.normal) / denominator;
    if (t > 0.f) {
      if (out_t) {
        *out_t = t;
      }
      return true;
    }
  }
  return false;
}

bool ray_hit_sphere(F32* out_t, Ray_t r, Sphere_t s) {
  // This equation of t has roots: (r.o + t*r.d.x - s.c.x)^2 + ... = s.r^2
  F32 temp_x = r.origin.x - s.center.x;
  F32 temp_y = r.origin.y - s.center.y;
  F32 temp_z = r.origin.z - s.center.z;
  F32 a = r.dir.x * r.dir.x + r.dir.y * r.dir.y + r.dir.z * r.dir.z;
  F32 b = r.dir.x * temp_x + r.dir.y * temp_y + r.dir.z * temp_z;
  F32 c = temp_x * temp_x + temp_y * temp_y + temp_z * temp_z - s.radius * s.radius;
  F32 delta = b * b - a * c;
  if (delta < 0.f)
    return false;
  // Both roots have to be positive.
  if (-b / a >= 0.f && c / a >= 0.f) {
    if (out_t) {
      *out_t = (-b - sqrt(delta)) / a;
    }
    return true;
  }
  return false;
}

bool ray_hit_triangle(Ray_t r, Triangle_t tri) {
  // Möller-Trumbore
  // Same variable names as the original paper.
  V3_t e1 = tri.vertices[1] - tri.vertices[0];
  V3_t e2 = tri.vertices[2] - tri.vertices[0];
  V3_t p = v3_cross(r.dir, e2);
  F32 det = v3_dot(e1, p);
  if (float_equal_0(det))
    return false;
  F32 inv_det = 1.f / det;
  V3_t t = r.origin - tri.vertices[0];
  F32 u = V3_t(t, p) * inv_det;
  if (u < 0.f || u > 1.f)
    return false;
  V3_t q = v3_cross(t, e1);
  F32 v = v3_dot(r.dir, q) * inv_det;
  if (v < 0.f || u + v > 1.f)
    return false;
  return true;
}
