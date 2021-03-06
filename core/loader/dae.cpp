//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/loader/dae.h"

#include "core/file.h"
#include "core/linear_allocator.h"
#include "core/log.h"

#include <ctype.h>
#include <stdlib.h>

struct Xml_node_t_ {
  char* tag_name;
  char* text;
  Dynamic_array_t<char*> attr_names;
  Dynamic_array_t<char*> attr_vals;
  Dynamic_array_t<struct Xml_node_t_*> children;
};

static char* alloc_string_(Allocator_t* allocator, const char* start, const char* end) {
  int len = end - start;
  char* str = (char*)allocator->alloc(end - start + 1);
  memcpy(str, start, len);
  str[len] = 0;
  return str;
}

static Xml_node_t_* parse_xml_(const char** last_pos, Allocator_t* allocator, const char* start, const char* end) {
  const char* p = start;
  Xml_node_t_* node = (Xml_node_t_*)allocator->alloc(sizeof(Xml_node_t_));
  node->text = NULL;
  while (p != end) {
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
      const char* tag_p = tag_end - 1;
      while(tag_p > tag_start && isspace(*tag_p)) {
        --tag_p;
      }
      if (*tag_p == '/') {
        tag_end = tag_p;
      } else {
        node->children.init(allocator);
      }

      // Parse tag_name
      tag_p = tag_start;
      while(tag_p != tag_end && isspace(*tag_p)) {
        ++tag_p;
      }
      const char* tag_name_start = tag_p;
      while (tag_p != tag_end && !isspace(*tag_p)) {
        ++tag_p;
      }
      const char* tag_name_end = tag_p;
      node->tag_name = alloc_string_(allocator, tag_name_start, tag_name_end);

      // Parse attribute
      while (tag_p != tag_end) {
        // attribute name
        while(tag_p != tag_end && isspace(*tag_p)) {
          ++tag_p;
        }
        if (tag_p == tag_end) {
          break;
        }

        if (node->attr_names.len() == 0) {
          node->attr_names.init(allocator);
          node->attr_vals.init(allocator);
        }
        const char* a_name_start = tag_p;
        while(tag_p != tag_end && !isspace(*tag_p) && *tag_p != '=') {
          ++tag_p;
        }
        const char* a_name_end = tag_p;
        char* a_name = alloc_string_(allocator, a_name_start, a_name_end);
        node->attr_names.append(a_name);
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
        node->attr_vals.append(a_val);
        ++tag_p;
      }

      if (node->children.len() == 0) {
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
        node->text = alloc_string_(allocator, text_start, text_end);
        p = text_end + 1;

        const char* closing_name_start = ++p;
        const char* closing_name_end = (const char*)memchr(p, '>', end - p);
        M_check_log_return_val(closing_name_end, NULL, "Can't find closing bracket of closing tag name");
        M_check_log_return_val(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->tag_name, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
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
          M_check_log_return_val(closing_name_end - closing_name_start && !memcmp(closing_name_start, node->tag_name, closing_name_end - closing_name_start), NULL, "Unmatched closing tag name");
          if (last_pos) {
            *last_pos = closing_name_end;
          }
          return node;
        }

        node->children.append(parse_xml_(&p, allocator, opening_bracket, end));
        ++p;
      }
    }
  }
  return node;
}

static Xml_node_t_* dae_find_node_(Xml_node_t_* node, const char* name) {
  Xml_node_t_* curr = node;
  while (1) {
    int name_len = strlen(name);
    const char* slash = (const char*)memchr(name, '/', name_len);
    int sub_elem_len = slash ? slash - name : strlen(name);
    for (int i = 0; i < curr->children.len(); ++i) {
      Xml_node_t_* child = curr->children[i];
      if (sub_elem_len == strlen(child->tag_name) && !memcmp(name, child->tag_name, sub_elem_len)) {
        curr = child;
        break;
      }
    }
    // Can't find sub element.
    if (!node) {
      return NULL;
    }
    // Final subelement.
    if (!slash) {
      break;
    }
    name = slash + 1;
  };
  return curr;
}

bool Dae_loader_t::init(Allocator_t* allocator, const Os_char* path) {
  Linear_allocator_t<> file_allocator("xml_file_allocator");
  file_allocator.init();
  Dynamic_array_t<U8> buffer = File_t::read_whole_file_as_text(&file_allocator, path);
  Xml_node_t_* root = parse_xml_(NULL, allocator, (char*)&buffer[0], (char*)&buffer[0] + buffer.len());
  file_allocator.destroy();

  Xml_node_t_* mesh_position = dae_find_node_(root, "library_geometries/geometry/mesh/source/float_array");
  int arr_len = atoi(mesh_position->attr_vals[1]);
  M_check_log_return_val(arr_len % 3 == 0, false, "Invalid vertex number");
  m_vertices.init(allocator);
  m_vertices.reserve(arr_len / 3);
  char* arr_p = mesh_position->text;
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
}
