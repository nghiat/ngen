//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/linear_allocator.h"
#include "core/loader/dae.h"
#include "core/path.h"
#include "core/path_utils.h"

int main(int argc, char** argv) {
  core_init(M_txt("dae_sample.log"));
  Linear_allocator_t<> allocator("allocator");
  Dae_loader_t dae(&allocator);
  dae.init(g_exe_dir.join(M_txt("assets/pirate.dae")));
  core_destroy();
  return 0;
}
