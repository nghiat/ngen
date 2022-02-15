//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/os.h"
#include "core/types.h"

template <typename T>
class String_t_ {
public:
  String_t_();
  String_t_(T& c);
  String_t_(T* str);
  String_t_(T* str, Sip len);

  bool equals(const String_t_<const T>& str) const;
  bool ends_with(const String_t_<const T>& str) const;
  String_t_<T> find_substr_(const String_t_<const T>& substr) const;
  String_t_<T> find_char_(T c) const;
  String_t_<T> find_char_reverse_(T c) const;

  String_t_<T> get_substr_from_offset_pointer_(T* p) const;

  T* m_p = NULL;
  Sip m_length = 0;
};

template <typename T_char, typename T_string>
class String_crtp_t_ {
public:
  T_string find_substr(const String_t_<const T_char>& substr) const;
  T_string find_char(T_char c) const;
  T_string find_char_reverse(T_char c) const;
};

template <typename T>
class Const_string_t_ : public String_t_<T>, public String_crtp_t_<T, Const_string_t_<T>> {
public:
  using String_t_<T>::String_t_;

  Const_string_t_<T> convert_string_t_substr_to_this_(const String_t_<T>& str) const;
};

template <typename T>
class Mutable_string_t_ : public String_t_<T>, public String_crtp_t_<T, Mutable_string_t_<T>> {
public:
  Mutable_string_t_();
  Mutable_string_t_(T* str);
  Mutable_string_t_(T* str, Sip cap);

  operator String_t_<const T>() const;

  void replace(T c, T new_c);
  void append(const String_t_<const T>& str);
  void copy(const String_t_<const T>& str);

  Mutable_string_t_<T> convert_string_t_substr_to_this_(const String_t_<T>& str) const;
  void append_null_if_possible();
  Sip m_capacity = 0;

private:
  Mutable_string_t_(const String_t_<T>& str);
};

using Os_cstring_t = Const_string_t_<const Os_char>;
using Os_mstring_t = Mutable_string_t_<Os_char>;

using Cstring_t = Const_string_t_<const char>;
using Mstring_t = Mutable_string_t_<char>;

#if M_os_is_win()
using Cwstring_t = Const_string_t_<const wchar_t>;
using Mwstring_t = Mutable_string_t_<wchar_t>;
#endif
