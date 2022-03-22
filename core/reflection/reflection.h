#pragma once

#define R_class __attribute__((annotate("reflected")))
#define R_field __attribute__((annotate("reflected")))
#define R_method __attribute__((annotate("reflected")))

#include "core/dynamic_array.h"
#include "core/string.h"

class Class_info_t {
public:
  bool has_field(const Cstring_t& name);
  Cstring_t m_class_name;
  Dynamic_array_t<Cstring_t> m_fields;
};

template <typename T>
Class_info_t* get_class_info();
