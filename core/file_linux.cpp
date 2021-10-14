//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/log.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void ngFile::f_delete_path(const char* path) {
  CHECK_RETURN(path);
  ::unlink(path);
}

bool ngFile::f_open(const char* path, EFileMode mode) {
  CHECK_RETURN_VAL(path, false);

  m_path = path;
  int flags = 0;
  if (mode & EFILE_MODE_READ)
    flags |= O_RDONLY;
  if (mode & EFILE_MODE_WRITE)
    flags |= O_RDWR | O_CREAT | O_TRUNC;
  if (mode & EFILE_MODE_APPEND)
    flags |= O_APPEND | O_RDWR;
  int modes = S_IRWXU;
  m_handle = ::open(m_path, flags, modes);
  return f_is_valid();
}

void ngFile::f_close() {
  CHECK_RETURN(f_is_valid());
  if (!::close(m_handle)) {
    m_handle = -1;
  }
}

void ngFile::f_delete() {
  CHECK_RETURN(f_is_valid());
  f_delete_path(m_path);
}

bool ngFile::f_read_plat(void* buffer, SIP* bytes_read, SIP size) {
  CHECK_RETURN_VAL(f_is_valid(), false);
  SIP rv = ::read(m_handle, buffer, size);
  if (bytes_read) {
    *bytes_read = rv;
  }
  return rv;
}

bool ngFile::f_write_plat(SIP* bytes_written, const void* buffer, SIP size) {
  CHECK_RETURN(f_is_valid());
  SIP rv = ::write(m_handle, buffer, size);
  if (rv == -1) {
    return false;
  }
  if (bytes_written) {
    *bytes_written = rv;
  }
  return true;
}

void ngFile::f_seek_plat(EFileFrom from, SIP distance) {
  CHECK_RETURN(f_is_valid());
  int whence;
  switch (from) {
  case EFILE_FROM_BEGIN:
    whence = SEEK_SET;
    break;
  case EFILE_FROM_CURRENT:
    whence = SEEK_CUR;
    break;
  case EFILE_FROM_END:
    whence = SEEK_END;
    break;
  }
  lseek(m_handle, distance, whence);
}

SIP ngFile::f_get_pos() const {
  CHECK_RETURN_VAL(f_is_valid(), F_INVALID_POS);
  return lseek(m_handle, 0, SEEK_CUR);
}

bool ngFile::f_is_valid() const {
  CHECK_RETURN_VAL(file, false);
  return m_handle != -1;
}

SIP ngFile::f_get_size() const {
  CHECK_RETURN_VAL(f_is_valid(), F_INVALID_SIZE);
  struct stat st;
  ::stat(m_path, &st);
  return st.st_size;
}
