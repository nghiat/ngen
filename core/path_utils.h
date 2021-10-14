//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/ng_types.h"

#define MAX_PATH_LEN (256)

bool path_utils_init();
OSChar* path_from_exe_dir(OSChar* out, const OSChar* sub_path, int len);
