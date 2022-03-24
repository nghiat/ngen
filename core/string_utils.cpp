//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/string_utils.h"

#include "core/dynamic_array.inl"
#include "core/hash_table.inl"

template <>
const char* c_or_w_(const char* c, const wchar_t* w) {
  return c;
}

template <>
const wchar_t* c_or_w_(const char* c, const wchar_t* w) {
  return w;
}

template <>
char c_or_w_(char c, wchar_t w) {
  return c;
}

template <>
wchar_t c_or_w_(char c, wchar_t w) {
  return w;
}

template <typename T>
bool find_next_brace_pair(const Cstring_t_<T>& str, int from, int* start_idx, int* end_idx) {
  for (int i = from; i < str.m_length; ++i) {
    T c = str.m_p[i];
    T next_i_c = 0;
    if (i + 1 < str.m_length) {
      next_i_c = str.m_p[i + 1];
    }
    if (c == M_c_or_w(T, '{') && next_i_c == M_c_or_w(T, '{')) {
      for (int j = i + 1; j < str.m_length; ++j) {
        T prev_j_c = str.m_p[j - 1];
        if (str.m_p[j] == M_c_or_w(T, '}') && prev_j_c == M_c_or_w(T, '}')) {
          *start_idx = i;
          *end_idx = j;
          M_check_log(j - i >= 4, "We don't support empty brace pair");
          return true;
        }
      }
      M_logf_return_val(false, "Can't find matching closing braces");
    }
  }
  return false;
}

template <typename T>
Hash_map<Cstring_t_<T>, Cstring_t_<T>> string_format_setup(Allocator_t* allocator, const Cstring_t_<T>& format, int* o_brace_pair_count) {
  int brace_pair_count = 0;
  int start_idx = 0;
  int end_idx = 0;
  while (find_next_brace_pair(format, end_idx, &start_idx, &end_idx)) {
    ++brace_pair_count;
  }
  Hash_map<Cstring_t_<T>, Cstring_t_<T>> dict;
  dict.init(allocator);
  dict.reserve(brace_pair_count);
  maybe_assign(o_brace_pair_count, brace_pair_count);
  return dict;
}

template Hash_map<Cstring_t_<char>, Cstring_t_<char>> string_format_setup(Allocator_t* allocator, const Cstring_t_<char>& format, int* o_brace_pair_count);
template Hash_map<Cstring_t_<wchar_t>, Cstring_t_<wchar_t>> string_format_setup(Allocator_t* allocator, const Cstring_t_<wchar_t>& format, int* o_brace_pair_count);

template <typename T>
Mstring_t_<T> string_format(Allocator_t* allocator, const Cstring_t_<T>& format, Hash_map<Cstring_t_<T>, Cstring_t_<T>>& dict) {
  int start_idx = 0;
  int end_idx = 0;
  int last_end_idx = 0;
  Dynamic_array_t<T> buffer;
  buffer.init(allocator);

  while (find_next_brace_pair(format, end_idx, &start_idx, &end_idx)) {
    if (start_idx > last_end_idx) {
      buffer.append_array(format.m_p + last_end_idx, start_idx - last_end_idx);
    }
    Cstring_t_<T> key = format.get_substr_from_offset(start_idx + 2, end_idx - start_idx - 3);
    Cstring_t_<T>* value = dict.find(key);
    if (!value) {
      M_logf("Can't find key \"%.*s\" in dict", key.m_length, key.m_p);
      buffer.destroy();
      return Mstring_t_<T>();
    }
    buffer.append_array(value->m_p, value->m_length);
    last_end_idx = end_idx + 1;
  }
  start_idx = format.m_length;
  if (start_idx > last_end_idx) {
    buffer.append_array(format.m_p + last_end_idx, start_idx - last_end_idx);
  }
  buffer.append(0);
  return Mstring_t_<T>(buffer.m_p, buffer.m_length - 1, buffer.m_capacity);
}

template Mstring_t_<char> string_format(Allocator_t* allocator, const Cstring_t_<char>& format, Hash_map<Cstring_t_<char>, Cstring_t_<char>>& dict);
template Mstring_t_<wchar_t> string_format(Allocator_t* allocator, const Cstring_t_<wchar_t>& format, Hash_map<Cstring_t_<wchar_t>, Cstring_t_<wchar_t>>& dict);
