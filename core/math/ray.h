//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/math/plane.h"
#include "core/math/sphere.h"
#include "core/math/triangle.h"
#include "core/math/vec3.h"

struct Ray {
  V3 origin;
  V3 dir;
};

V3 ray_at(const Ray& r, F32 t);
bool ray_hit_plane(F32* out_t, const Ray& r, const ngPlane& p);
bool ray_hit_sphere(F32* out_t, const Ray& r, const Sphere& s);
bool ray_hit_triangle(const Ray& r, const Triangle& t);
