//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"
#include "core/hash.h"
#include "core/types.h"

template <typename T>
class String_t_ {
public:
  String_t_();
  String_t_(T& c);
  String_t_(T* str);
  String_t_(T* str, Sip len);
  String_t_(T* str, Sip len, Sip capacity);
  String_t_(T* from, T* to);

  bool operator==(const String_t_<const T>& str) const;
  bool equals(const String_t_<const T>& str) const;
  bool ends_with(const String_t_<const T>& str) const;
  bool find_substr(Sip* o_index, const String_t_<const T>& substr, Sip from = 0) const;
  bool find_char(Sip* o_index, T c, Sip from = 0) const;
  bool find_char_reverse(Sip* o_index, T c, Sip from = -1) const;
  int count(const String_t_<const T>& str) const;
  String_t_<T> get_substr(Sip from, Sip to = -1) const;
  String_t_<const T> to_const() const;

  void replace(T c, T new_c, Sip from = 0);
  void append(const String_t_<const T>& str);
  void copy(const String_t_<const T>& str);

  T* m_p = NULL;
  Sip m_length = 0;
  Sip m_capacity = 0;
};

template <typename T>
struct Hash_t<String_t_<T>> {
  Sz operator()(const String_t_<T>& key) const;
};

using Os_cstring_t = String_t_<const Os_char>;
using Os_mstring_t = String_t_<Os_char>;

template <typename T>
using Cstring_t_ = String_t_<const T>;
template <typename T>
using Mstring_t_ = String_t_<T>;

using Cstring_t = String_t_<const char>;
using Mstring_t = String_t_<char>;

#if M_os_is_win()
using Cwstring_t = String_t_<const wchar_t>;
using Mwstring_t = String_t_<wchar_t>;
#endif

#include "core/string.inl"
