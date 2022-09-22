//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/xml.h"

#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/intrusive_list.inl"
#include "core/linear_allocator.h"
#include "core/utils.h"

#include <ctype.h>
#include <stdlib.h>

static char* alloc_string_(Allocator_t* allocator, const char* start, const char* end) {
  int len = end - start;
  char* str = (char*)allocator->alloc(end - start + 1);
  memcpy(str, start, len);
  str[len] = 0;
  return str;
}

static Xml_node_t* parse_xml_(const char** last_pos, Allocator_t* allocator, const char* start, const char* end) {
  const char* p = start;
  Xml_node_t* node = NULL;
  while (p != end) {
    if (!node) {
      node = allocator->construct<Xml_node_t>(allocator);
    }
    while (p != end && *p != '<') {
      ++p;
    }
    if (*p == '<') {
      const char* tag_start = p + 1;
      if (*tag_start == '?') {
        p = (const char*)memchr(p, '>', end - p);
        M_check_log_return_val(p, NULL, "Can't find closing bracket for metadata tag");
        continue;
      }
      while (*(++p) != '>') {}
      const char* tag_end = p;
      ++p;
      // Check self-closing tag
      bool is_self_closing = false;
      const char* tag_p = tag_end - 1;
      while(tag_p > tag_start && isspace(*tag_p)) {
        --tag_p;
      }
      if (*tag_p == '/') {
        tag_end = tag_p;
        is_self_closing = true;
      }

      // Parse m_tag_name
      tag_p = tag_start;
      while(tag_p != tag_end && isspace(*tag_p)) {
        ++tag_p;
      }
      const char* m_tag_name_start = tag_p;
      while (tag_p != tag_end && !isspace(*tag_p)) {
        ++tag_p;
      }
      const char* m_tag_name_end = tag_p;
      node->m_tag_name = alloc_string_(allocator, m_tag_name_start, m_tag_name_end);

      // Parse attribute
      while (tag_p != tag_end) {
        // attribute name
        while(tag_p != tag_end && isspace(*tag_p)) {
          ++tag_p;
        }
        if (tag_p == tag_end) {
          break;
        }

        const char* a_name_start = tag_p;
        while(tag_p != tag_end && !isspace(*tag_p) && *tag_p != '=') {
          ++tag_p;
        }
        const char* a_name_end = tag_p;
        char* a_name = alloc_string_(allocator, a_name_start, a_name_end);
        node->m_attr_names.append(a_name);
        ++tag_p;

        // attribute val
        while(tag_p != tag_end && *tag_p != '"') {
          ++tag_p;
        }
        const char* a_val_start = ++tag_p;
        while(tag_p != tag_end && *tag_p != '"') {
          ++tag_p;
        }
        const char* a_val_end = tag_p;
        char* a_val = alloc_string_(allocator, a_val_start, a_val_end);
        node->m_attr_vals.append(a_val);
        ++tag_p;
      }

      if (is_self_closing) {
        if (last_pos) {
          *last_pos = p;
        }
        return node;
      }

      // Parse content
      while (p != end && isspace(*p)) {
        ++p;
      }

      // It's the text.
      if (*p != '<') {
        const char* text_start = p;
        const char* text_end = (const char*)memchr(p, '<', end - text_start);
        M_check_log_return_val(text_end, NULL, "Can't find closing tag");
        node->m_text = alloc_string_(allocator, text_start, text_end);
        p = text_end + 1;

        const char* closing_name_start = ++p;
        const char* closing_name_end = (const char*)memchr(p, '>', end - p);
        M_check_log_return_val(closing_name_end, NULL, "Can't find closing bracket of closing tag name");
        M_check_log_return_val(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->m_tag_name.m_p, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
        if (last_pos) {
          *last_pos = closing_name_end;
        }
        return node;
      }

      // It's the children
      while (p != end) {
        while (p != end && isspace(*p)) {
          ++p;
        }
        const char* opening_bracket = p;
        M_check_log_return_val(*p == '<', NULL, "This codepath processes children or closing tag which have to start with <");
        ++p;
        // It's closing tag.
        if (*p == '/') {
          const char* closing_name_start = ++p;
          const char* closing_name_end = (const char*)memchr(p, '>', end - p);
          M_check_log_return_val(closing_name_end, NULL, "Can't find closing bracket of closing tag name");
          M_check_log_return_val(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->m_tag_name.m_p, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
          if (last_pos) {
            *last_pos = closing_name_end;
          }
          return node;
        }

        Xml_node_t* child = parse_xml_(&p, allocator, opening_bracket, end);
        node->m_children.append(child);
        ++p;
      }
    }
  }
  return node;
}

const Xml_node_t* Xml_node_t::find_child(const Cstring_t& name) const {
  const Xml_node_t* rv = NULL;
  const Xml_node_t* curr = this;
  Cstring_t curr_name = name;
  while (true) {
    // const char* slash = (const char*)memchr(name, '/', name_len);
    Cstring_t slash = curr_name.find_char('/');
    Cstring_t child_name;
    bool is_final = false;
    if (slash.m_length) {
      child_name = curr_name.get_substr_till(slash);
    } else {
      child_name = curr_name;
      is_final = true;
    }
    bool found_child = false;
    for (const auto& child : curr->m_children) {
      if (child->m_tag_name.equals(child_name)) {
        curr = child;
        found_child = true;
        break;
      }
    }
    // Final subelement.
    if (is_final) {
      if (found_child) {
        rv = curr;
      }
      break;
    }
    curr_name = slash.get_substr_from_offset(1);
  }
  return rv;
}

const Xml_node_t* Xml_node_t::find_id(const Cstring_t& id) const {
  const Xml_node_t* curr = this;
  for (const auto& child : curr->m_children) {
    for (int i = 0; i < child->m_attr_names.len(); ++i) {
      if (child->m_attr_names[i].equals("id")) {
        if (child->m_attr_vals[i].equals(id)) {
          return child;
        }
        break;
      }
    }
    const Xml_node_t* child_rv = child->find_id(id);
    if (child_rv) {
      return child_rv;
    }
  }
  return NULL;
}

void Xml_node_t::destory() {
}

bool Xml_t::init(const Path_t& path) {
  Linear_allocator_t<> file_allocator("xml_file_allocator");
  M_scope_exit(file_allocator.destroy());
  Dynamic_array_t<U8> buffer = File_t::read_whole_file_as_text(&file_allocator, path.m_path);
  return init((char*)buffer.m_p, buffer.m_length);
}

bool Xml_t::init(const char* buffer, int length) {
  m_root = parse_xml_(NULL, m_allocator, buffer, buffer + length);
  return true;
}

void Xml_t::destroy() {
}