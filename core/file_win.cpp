//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/log.h"
#include "core/utils.h"

#include <Windows.h>

void ngFile::f_delete_path(const wchar_t* path) {
  CHECK_RETURN(path);
  DeleteFile(path);
}

bool ngFile::f_open_plat(const wchar_t* path, EFileMode mode) {
  CHECK_RETURN_VAL(path, false);

  m_path = path;
  DWORD access = 0;
  if (mode & EFILE_MODE_READ)
    access |= GENERIC_READ;
  if (mode & EFILE_MODE_WRITE)
    access |= GENERIC_READ | GENERIC_WRITE;
  if (mode & EFILE_MODE_APPEND)
    access |= GENERIC_READ | FILE_APPEND_DATA;

  DWORD share_mode = 0;
  share_mode |= FILE_SHARE_READ;

  DWORD create_disposition = 0;
  if (mode & EFILE_MODE_READ)
    create_disposition = OPEN_EXISTING;
  if (mode & EFILE_MODE_WRITE)
    create_disposition = CREATE_ALWAYS;
  if (mode & EFILE_MODE_APPEND)
    create_disposition = OPEN_ALWAYS;
  m_handle = CreateFile(
      m_path, access, share_mode, NULL, create_disposition, 0, NULL);
  CHECK_LOG_RETURN_VAL(f_is_valid(), false, "Can't open file %ls", path);
  return true;
}

void ngFile::f_close_plat() {
  CHECK_RETURN(f_is_valid());
  CloseHandle(m_handle);
  m_handle = INVALID_HANDLE_VALUE;
}

void ngFile::f_delete() {
  CHECK_RETURN(f_is_valid());
  f_delete_path(m_path);
}

bool ngFile::f_read_plat(void* buffer, SIP* bytes_read, SIP size) {
  CHECK_RETURN_VAL(f_is_valid(), false);
  DWORD read = 0;
  ReadFile(m_handle, buffer, size, &read, NULL);
  if (bytes_read) {
    *bytes_read = read;
  }
  return read;
}

bool ngFile::f_write_plat(SIP* bytes_written, const void* buffer, SIP size) {
  CHECK_RETURN_VAL(f_is_valid(), false);
  DWORD bytes_written_plat = 0;
  bool rv = WriteFile(m_handle, buffer, size, &bytes_written_plat, NULL);
  maybe_assign(bytes_written, (SIP)bytes_written_plat);
  return rv;
}

void ngFile::f_seek_plat(EFileFrom from, SIP distance) {
  CHECK_RETURN(f_is_valid());
  DWORD move_method;
  switch (from) {
  case EFILE_FROM_BEGIN:
    move_method = FILE_BEGIN;
    break;
  case EFILE_FROM_CURRENT:
    move_method = FILE_CURRENT;
    break;
  case EFILE_FROM_END:
    move_method = FILE_END;
    break;
  }
  SetFilePointer(m_handle, distance, NULL, move_method);
}

SIP ngFile::f_get_pos() const {
  CHECK_RETURN_VAL(f_is_valid(), F_INVALID_POS);
  return SetFilePointer(m_handle, 0, NULL, FILE_CURRENT);
}

bool ngFile::f_is_valid() const {
  return m_handle != INVALID_HANDLE_VALUE;
}

SIP ngFile::f_get_size() const {
  CHECK_RETURN_VAL(f_is_valid(), F_INVALID_SIZE);
  return GetFileSize(m_handle, NULL);
}
