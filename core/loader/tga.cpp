//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/loader/tga.h"

#include "core/file.h"
#include "core/log.h"
#include "core/utils.h"

struct ColormapSpec {
  U16 first_entry_index;
  U16 color_map_length;
  U8 color_map_entry_size;
} ;

struct ImageSpec {
  U16 x_origin;
  U16 y_origin;
  U16 width;
  U16 height;
  U8 depth;
  U8 descriptor;
};

bool tga_write(const U8* data, int width, int height, const OSChar* path) {
  ngFile f;
  CHECK_LOG_RETURN_VAL(f.f_open(path, EFILE_MODE_WRITE), false, "Can't open " OS_TXT_PR " to write tga",  path);
  SCOPE_EXIT(f.f_close());
  {
    uint8_t id_length = 0;
    f.f_write(NULL, &id_length, sizeof(id_length));
  }
  {
    uint8_t color_map_type = 0;
    f.f_write(NULL, &color_map_type, sizeof(color_map_type));
  }
  {
    uint8_t image_type = 2;
    f.f_write(NULL, &image_type, sizeof(image_type));
  }
  {
    ColormapSpec spec = {};
    f.f_write(NULL, &spec, sizeof(spec));
  }
  {
    ImageSpec spec;
    spec.x_origin = 0;
    spec.y_origin = 0;
    spec.width = width;
    spec.height = height;
    spec.depth = 24;
    spec.descriptor = 0;
    f.f_write(NULL, &spec, sizeof(spec));
  }
  f.f_write(NULL, data, width * height * 3);
  return true;
}
