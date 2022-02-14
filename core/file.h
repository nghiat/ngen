//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/os.h"
#include "core/types.h"

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

struct Allocator_t;
struct File_buffer_t;

class File_t {
public:
  static bool init();
  static void delete_path(const Os_char* path);
  static Dynamic_array_t<U8> read_whole_file_as_text(Allocator_t* allocator, const Os_char* path);

  bool open(const Os_char* path, enum E_file_mod mode);
  void close();

  void delete_this();

  bool read(void* buffer, Sip* bytes_read, Sip size);
  bool read_line(char* buffer, Sip size);
  bool write(Sip* bytes_written, const void* in, Sip size);
  void seek(enum E_file_from from, Sip distance);
  void flush();

  Sip get_pos() const;
  bool is_valid() const;
  Sip get_size() const;

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
  File_buffer_t* m_internal_buffer = NULL;

private:
  bool open_plat_(const Os_char* path, E_file_mod mode);
  void close_plat_();

  bool read_plat_(void* buffer, Sip* bytes_read, Sip size);
  bool write_plat_(Sip* bytes_written, const void* buffer, Sip size);
  void seek_plat_(E_file_from from, Sip distance);
};
