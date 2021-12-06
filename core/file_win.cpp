//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/log.h"
#include "core/utils.h"

#include <Windows.h>

void File::delete_path(const wchar_t* path) {
  M_check_return(path);
  DeleteFile(path);
}

bool File::open_plat_(const wchar_t* path, E_file_mod mode) {
  M_check_return_val(path, false);

  m_path = path;
  DWORD access = 0;
  if (mode & e_file_mode_read)
    access |= GENERIC_READ;
  if (mode & e_file_mode_write)
    access |= GENERIC_READ | GENERIC_WRITE;
  if (mode & e_file_mode_append)
    access |= GENERIC_READ | FILE_APPEND_DATA;

  DWORD share_mode = 0;
  share_mode |= FILE_SHARE_READ;

  DWORD create_disposition = 0;
  if (mode & e_file_mode_read)
    create_disposition = OPEN_EXISTING;
  if (mode & e_file_mode_write)
    create_disposition = CREATE_ALWAYS;
  if (mode & e_file_mode_append)
    create_disposition = OPEN_ALWAYS;
  m_handle = CreateFile(
      m_path, access, share_mode, NULL, create_disposition, 0, NULL);
  M_check_log_return_val(is_valid(), false, "Can't open file %ls", path);
  return true;
}

void File::close_plat_() {
  M_check_return(is_valid());
  CloseHandle(m_handle);
  m_handle = INVALID_HANDLE_VALUE;
}

void File::delete_this() {
  M_check_return(is_valid());
  delete_path(m_path);
}

bool File::read_plat_(void* buffer, Sip* bytes_read, Sip size) {
  M_check_return_val(is_valid(), false);
  DWORD read = 0;
  ReadFile(m_handle, buffer, size, &read, NULL);
  if (bytes_read) {
    *bytes_read = read;
  }
  return read;
}

bool File::write_plat_(Sip* bytes_written, const void* buffer, Sip size) {
  M_check_return_val(is_valid(), false);
  DWORD bytes_written_plat = 0;
  bool rv = WriteFile(m_handle, buffer, size, &bytes_written_plat, NULL);
  maybe_assign(bytes_written, (Sip)bytes_written_plat);
  return rv;
}

void File::seek_plat_(E_file_from from, Sip distance) {
  M_check_return(is_valid());
  DWORD move_method;
  switch (from) {
  case e_file_from_begin:
    move_method = FILE_BEGIN;
    break;
  case e_file_from_current:
    move_method = FILE_CURRENT;
    break;
  case e_file_from_end:
    move_method = FILE_END;
    break;
  }
  SetFilePointer(m_handle, distance, NULL, move_method);
}

Sip File::get_pos() const {
  M_check_return_val(is_valid(), F_INVALID_POS);
  return SetFilePointer(m_handle, 0, NULL, FILE_CURRENT);
}

bool File::is_valid() const {
  return m_handle != INVALID_HANDLE_VALUE;
}

Sip File::get_size() const {
  M_check_return_val(is_valid(), F_INVALID_SIZE);
  return GetFileSize(m_handle, NULL);
}
