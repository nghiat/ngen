//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/ng_types.h"
#include "core/os.h"

#if M_os_is_win()
#  include "core/windows_lite.h"
#endif

enum E_file_mod {
  // open file if it exists, otherwise, create a new file.
  e_file_mode_read = 1 << 0,
  e_file_mode_write = 1 << 1,
  e_file_mode_append = 1 << 2,
};

enum E_file_from {
  e_file_from_begin,
  e_file_from_current,
  e_file_from_end
};

struct Allocator;
struct FileBuffer;

class File {
public:
  static bool f_init();
  static void f_delete_path(const Os_char* path);
  static Dynamic_array<U8> f_read_whole_file_as_text(Allocator* allocator, const Os_char* path);

  bool f_open(const Os_char* path, enum E_file_mod mode);
  void f_close();

  void f_delete();

  bool f_read(void* buffer, Sip* bytes_read, Sip size);
  bool f_read_line(char* buffer, Sip size);
  bool f_write(Sip* bytes_written, const void* in, Sip size);
  void f_seek(enum E_file_from from, Sip distance);
  void f_flush();

  Sip f_get_pos() const;
  bool f_is_valid() const;
  Sip f_get_size() const;

  static const Sip F_INVALID_POS = -1;
  static const Sip F_INVALID_SIZE = -1;

#if M_os_is_win()
  HANDLE m_handle;
#elif M_os_is_linux()
  int m_handle;
#else
#error "?"
#endif
  const Os_char* m_path;
  FileBuffer* m_internal_buffer = NULL;

private:
  bool f_open_plat_(const Os_char* path, E_file_mod mode);
  void f_close_plat_();

  bool f_read_plat_(void* buffer, Sip* bytes_read, Sip size);
  bool f_write_plat_(Sip* bytes_written, const void* buffer, Sip size);
  void f_seek_plat_(E_file_from from, Sip distance);
};
