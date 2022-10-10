// This file is generated from {{source_header_path}}

#include "{{source_header_path}}"

#include "core/reflection/reflection.h"

#include "core/core_allocators.h"
#include "core/dynamic_array.h"
#include "core/string.h"
#include "core/string_utils.h"

Class_info_t g_class_info_;
bool g_is_init_ = false;

static void init_fields_() {
  g_class_info_.m_fields.init(g_general_allocator);
  g_class_info_.m_fields.reserve({{field_count}});
{{fields}}
}

template <>
Class_info_t* get_class_info<{{class_name}}>() {
  if (g_is_init_) {
    return &g_class_info_;
  }
  g_class_info_.m_class_name = string_printf(g_general_allocator, "%s", "{{class_name}}").to_const();
  init_fields_();
  g_is_init_ = true;
  return &g_class_info_;
}
