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

class Xml_node_t {
public:
  Xml_node_t(Allocator_t* allocator) : m_attr_names(allocator), m_attr_vals(allocator), m_children(allocator) {}
  void destory();

  const Xml_node_t* find_child(const Cstring_t& name) const;
  const Xml_node_t* find_id(const Cstring_t& id) const;

  Mstring_t m_tag_name;
  Mstring_t m_text;
  Dynamic_array_t<Mstring_t> m_attr_names;
  Dynamic_array_t<Mstring_t> m_attr_vals;
  Dynamic_array_t<Xml_node_t*> m_children;
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
