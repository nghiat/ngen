//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/tga.h"

#include "core/file.h"
#include "core/log.h"
#include "core/utils.h"

struct Colormap_spec_t_ {
  U16 first_entry_index;
  U16 color_map_length;
  U8 color_map_entry_size;
} ;

struct Image_spec_t_ {
  U16 x_origin;
  U16 y_origin;
  U16 width;
  U16 height;
  U8 depth;
  U8 descriptor;
};

bool tga_write(const U8* data, int width, int height, const Os_char* path) {
  File_t f;
  M_check_log_return_val(f.open(path, e_file_mode_write), false, "Can't open " M_txt_p " to write tga",  path);
  M_scope_exit(f.close());
  {
    uint8_t id_length = 0;
    f.write(NULL, &id_length, sizeof(id_length));
  }
  {
    uint8_t color_map_type = 0;
    f.write(NULL, &color_map_type, sizeof(color_map_type));
  }
  {
    uint8_t image_type = 2;
    f.write(NULL, &image_type, sizeof(image_type));
  }
  {
    Colormap_spec_t_ spec = {};
    f.write(NULL, &spec, sizeof(spec));
  }
  {
    Image_spec_t_ spec;
    spec.x_origin = 0;
    spec.y_origin = 0;
    spec.width = width;
    spec.height = height;
    spec.depth = 24;
    spec.descriptor = 0;
    f.write(NULL, &spec, sizeof(spec));
  }
  f.write(NULL, data, width * height * 3);
  return true;
}
