//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#pragma once

#include "core/dynamic_array.h"
#include "core/ng_types.h"
#include "core/os.h"

#if OS_WIN()
#  include "core/windows_lite.h"
#endif

enum EFileMode {
  // open file if it exists, otherwise, create a new file.
  EFILE_MODE_READ = 1 << 0,
  EFILE_MODE_WRITE = 1 << 1,
  EFILE_MODE_APPEND = 1 << 2,
};

enum EFileFrom {
  EFILE_FROM_BEGIN,
  EFILE_FROM_CURRENT,
  EFILE_FROM_END
};

struct ngAllocator;
struct FileBuffer;

class ngFile {
public:
  static bool f_init();
  static void f_delete_path(const OSChar* path);
  static DynamicArray<U8> f_read_whole_file_as_text(ngAllocator* allocator, const OSChar* path);

  bool f_open(const OSChar* path, enum EFileMode mode);
  void f_close();

  void f_delete();

  bool f_read(void* buffer, SIP* bytes_read, SIP size);
  bool f_read_line(char* buffer, SIP size);
  bool f_write(SIP* bytes_written, const void* in, SIP size);
  void f_seek(enum EFileFrom from, SIP distance);
  void f_flush();

  SIP f_get_pos() const;
  bool f_is_valid() const;
  SIP f_get_size() const;

  static const SIP F_INVALID_POS = -1;
  static const SIP F_INVALID_SIZE = -1;
private:
  bool f_open_plat(const OSChar* path, EFileMode mode);
  void f_close_plat();

  bool f_read_plat(void* buffer, SIP* bytes_read, SIP size);
  bool f_write_plat(SIP* bytes_written, const void* buffer, SIP size);
  void f_seek_plat(EFileFrom from, SIP distance);

#if OS_WIN()
  HANDLE m_handle;
#elif OS_LINUX()
  int m_handle;
#else
#error "?"
#endif
  const OSChar* m_path;
  FileBuffer* m_internal_buffer = NULL;
};
