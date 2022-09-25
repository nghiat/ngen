//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/hash_table.h"
#include "core/path.h"
#include "core/string.h"

struct Allocator_t;

class Xml_node_t;
typedef Dynamic_array_t<Xml_node_t*> Xml_nodes_t;

class Xml_node_t {
public:
  Xml_node_t(Allocator_t* allocator) : m_attributes(allocator), m_children(allocator) {}
  void destory();

  const Xml_node_t* find_first_by_path(const Cstring_t& name) const;
  const Xml_node_t* find_first_by_attr(const Cstring_t& name, const Cstring_t& val) const;
  const Xml_node_t* find_first_by_tag(const Cstring_t& name) const;
  void find_all_by_tag(Xml_nodes_t* nodes, const Cstring_t& name) const;
  int count_all_by_tag(const Cstring_t& name) const;

  Mstring_t m_tag_name;
  Mstring_t m_text;
  Hash_map_t<Cstring_t, Cstring_t> m_attributes;
  Xml_nodes_t m_children;
};

class Xml_t {
public:
  Xml_t(Allocator_t* allocator) : m_allocator(allocator) {}
  bool init(const Path_t& path);
  bool init(const char* buffer, int length);
  void destroy();
  Allocator_t* m_allocator = NULL;
  Xml_node_t* m_root = NULL;
};
