//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/linear_allocator.inl"
#include "core/loader/png.h"
#include "core/mono_time.h"
#include "core/path.h"
#include "core/path_utils.h"
#include "core/utils.h"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"

int main(int argc, char** argv) {
  core_init(M_txt("gpu_sample.log"));
  Path_t paths[] = {
    g_exe_dir.join(M_txt("assets/posx.png")),
    g_exe_dir.join(M_txt("assets/negx.png")),
    g_exe_dir.join(M_txt("assets/posy.png")),
    g_exe_dir.join(M_txt("assets/negy.png")),
    g_exe_dir.join(M_txt("assets/posz.png")),
    g_exe_dir.join(M_txt("assets/negz.png")),
    g_exe_dir.join(M_txt("assets/basecolor.png")),
    g_exe_dir.join(M_txt("assets/normal.png")),
    g_exe_dir.join(M_txt("assets/metallic.png")),
    g_exe_dir.join(M_txt("assets/roughness.png")),
  };
  Linear_allocator_t<> png_temp_allocator("png_temp_allocator");
  {
    auto start = mono_time_now();
    for (int i = 0; i < static_array_size(paths); ++i) {
      Scope_allocator_t<> scope_allocator(&png_temp_allocator);
      Png_loader_t png;
      png.init(&scope_allocator, paths[i]);
    }
    M_logi("%f s", mono_time_to_s(mono_time_now() - start));
  }
  {
    auto start = mono_time_now();
    int x,y,n;
    for (int i = 0; i < static_array_size(paths); ++i) {
      unsigned char *data = stbi_load(paths[i].get_path8().m_path, &x, &y, &n, 0);
    }
    M_logi("%f s", mono_time_to_s(mono_time_now() - start));
  }
  core_destroy();
  return 0;
}
