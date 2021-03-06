//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/log.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void File_t::delete_path(const char* path) {
  M_check_return(path);
  ::unlink(path);
}

bool File_t::open_plat_(const char* path, E_file_mod mode) {
  M_check_return_val(path, false);

  m_path = path;
  int flags = 0;
  if (mode & e_file_mode_read)
    flags |= O_RDONLY;
  if (mode & e_file_mode_write)
    flags |= O_RDWR | O_CREAT | O_TRUNC;
  if (mode & e_file_mode_append)
    flags |= O_CREAT | O_APPEND | O_RDWR;
  int modes = S_IRWXU;
  m_handle = ::open(m_path, flags, modes);
  return is_valid();
}

void File_t::close_plat_() {
  M_check_return(is_valid());
  if (!::close(m_handle)) {
    m_handle = -1;
  }
}

void File_t::delete_this() {
  M_check_return(is_valid());
  delete_path(m_path);
}

bool File_t::read_plat_(void* buffer, Sip* bytes_read, Sip size) {
  M_check_return_val(is_valid(), false);
  Sip rv = ::read(m_handle, buffer, size);
  if (bytes_read) {
    *bytes_read = rv;
  }
  return rv;
}

bool File_t::write_plat_(Sip* bytes_written, const void* buffer, Sip size) {
  M_check_return_val(is_valid(), false);
  Sip rv = ::write(m_handle, buffer, size);
  if (rv == -1) {
    return false;
  }
  if (bytes_written) {
    *bytes_written = rv;
  }
  return true;
}

void File_t::seek_plat_(E_file_from from, Sip distance) {
  M_check_return(is_valid());
  int whence;
  switch (from) {
  case e_file_from_begin:
    whence = SEEK_SET;
    break;
  case e_file_from_current:
    whence = SEEK_CUR;
    break;
  case e_file_from_end:
    whence = SEEK_END;
    break;
  }
  lseek(m_handle, distance, whence);
}

Sip File_t::get_pos() const {
  M_check_return_val(is_valid(), F_INVALID_POS);
  return lseek(m_handle, 0, SEEK_CUR);
}

bool File_t::is_valid() const {
  return m_handle != -1;
}

Sip File_t::get_size() const {
  M_check_return_val(is_valid(), F_INVALID_SIZE);
  struct stat st;
  ::stat(m_path, &st);
  return st.st_size;
}
