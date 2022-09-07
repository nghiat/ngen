//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/dae.h"

#include "core/dynamic_array.inl"
#include "core/linear_allocator.inl"
#include "core/loader/xml.h"
#include "core/log.h"
#include "core/utils.h"

bool Dae_loader_t::init(Allocator_t* allocator, const Path_t& path) {
  m_vertices.m_allocator = allocator;

  Linear_allocator_t<> xml_allocator("xml_allocator");
  M_scope_exit(xml_allocator.destroy());
  Xml_t xml(&xml_allocator);
  xml.init(path);
  const Xml_node_t* mesh_position = xml.find_node("library_geometries/geometry/mesh/source/float_array");
  int arr_len = atoi(mesh_position->attr_vals[1].m_p);
  M_check_log_return_val(arr_len % 3 == 0, false, "Invalid vertex number");
  m_vertices.reserve(arr_len / 3);
  char* arr_p = mesh_position->text.m_p;
  while (*arr_p) {
    float x = strtof(arr_p, &arr_p);
    float y = strtof(arr_p, &arr_p);
    float z = strtof(arr_p, &arr_p);
    m_vertices.append({x, y, z, 1.0f});
  }
  M_check_log_return_val(arr_len / 3 == m_vertices.len(), false, "Unmatched vertex number between attribute and text");
  return true;
}

void Dae_loader_t::destroy() {
  m_vertices.destroy();
}
