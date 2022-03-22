#include "core/reflection/reflection.h"

#include "core/dynamic_array.inl"

bool Class_info_t::has_field(const Cstring_t& name) {
  for (int i = 0; i < m_fields.len(); ++i) {
    if (name.equals(m_fields[i])) {
      return true;
    }
  }
  return false;
}
