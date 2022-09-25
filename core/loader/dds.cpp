//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/dds.h"

#include "core/file.h"
#include "core/log.h"
#include "core/utils.h"

bool Dds_loader_t::init(const Path_t& path) {
  m_file_data = File_t::read_whole_file_as_binary(m_file_data.m_allocator, path.m_path);
  U8* p = m_file_data.m_p;
  U32 magic_num = *(U32*)p;
  p += 4;
  M_check_return_val(magic_num == 0x20534444, false);
  m_header = (Dds_header_t*)p;
  M_check_return_val(m_header->size == 124, false);
  p += sizeof(Dds_header_t);
  M_check_log_return_val(m_header->pixel_format.four_cc == four_cc("DX10"), false, "Howelse do we check the format");
  m_header10 = (Dds_header_dxt10_t*)p;
  switch(m_header10->dxgi_format) {
    case e_dxgi_format_bc7_unorm:
      m_format = e_format_bc7_unorm;
      break;
    default:
      M_unimplemented();
  }
  p+= sizeof(Dds_header_dxt10_t);
  m_data = p;
  return true;
}

void Dds_loader_t::destroy() {
  m_file_data.destroy();
}
