//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/path.h"
#include "core/string.h"

struct Allocator_t;

struct Xml_node_t {
  Xml_node_t(Allocator_t* allocator) : attr_names(allocator), attr_vals(allocator), children(allocator) {}
  void destory();

  Mstring_t tag_name;
  Mstring_t text;
  Dynamic_array_t<Mstring_t> attr_names;
  Dynamic_array_t<Mstring_t> attr_vals;
  Dynamic_array_t<Xml_node_t*> children;
};

class Xml_t {
public:
  Xml_t(Allocator_t* allocator) : m_allocator(allocator) {}
  bool init(const Path_t& path);
  bool init(const char* buffer, int length);
  void destroy();
  const Xml_node_t* find_node(const char* name, const Xml_node_t* parent = NULL);
  Allocator_t* m_allocator = NULL;
  Xml_node_t* m_root = NULL;
};
