//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#include "core/file.h"

#include "core/allocator.h"
#include "core/core_allocators.h"
#include "core/dynamic_array.inl"
#include "core/log.h"
#include "core/utils.h"

#include <string.h>

static const int gc_max_buffer = 16;
static const int gc_max_buffer_size = 8 * 1024;

struct FileBuffer {
  U8* buffer = NULL;
  SZ len = 0;
  // offset from the start of the buffer.
  SZ offset = 0;
  bool is_in_used : 1;
  bool is_writing : 1;
};

static FileBuffer g_buffers[gc_max_buffer];

bool ngFile::f_init() {
  for (int i = 0; i < gc_max_buffer; ++i) {
    g_buffers[i].buffer = (U8*)g_persistent_allocator->al_alloc(gc_max_buffer_size);
    g_buffers[i].is_in_used = false;
    if (!g_buffers[i].buffer) {
      return false;
    }
  }

  return true;
}

DynamicArray<U8> ngFile::f_read_whole_file_as_text(ngAllocator* allocator, const OSChar* path) {
  DynamicArray<U8> buffer;
  ngFile f;
  f.f_open(path, EFILE_MODE_READ);
  CHECK_RETURN_VAL(f.f_is_valid(), buffer);
  SIP file_size = f.f_get_size();
  buffer.da_init(allocator);
  buffer.da_resize(file_size + 1);
  f.f_read_plat(&buffer[0], NULL, file_size);
  buffer[file_size] = 0;
  f.f_close();
  return buffer;
}

bool ngFile::f_open(const OSChar* path, enum EFileMode mode) {
  bool rv = f_open_plat(path, mode);
  if (!rv) {
    return false;
  }
  for (int i = 0; i < gc_max_buffer; ++i) {
    if (!g_buffers[i].is_in_used) {
      g_buffers[i].len = 0;
      g_buffers[i].offset = 0;
      g_buffers[i].is_in_used = true;
      g_buffers[i].is_writing = false;
      m_internal_buffer = &g_buffers[i];
      break;
    }
  }
  CHECK_LOG_RETURN_VAL(m_internal_buffer, false, "Can't get a file buffer, too many files are being opened");
  return true;
}

void ngFile::f_close() {
  f_flush();
  f_close_plat();
  if (m_internal_buffer) {
    for (int i = 0; i < gc_max_buffer; ++i) {
      if (m_internal_buffer == &g_buffers[i]) {
        g_buffers[i].is_in_used = false;
        m_internal_buffer = NULL;
        break;
      }
    }
  }
  CHECK_RETURN(!m_internal_buffer);
}

bool ngFile::f_read(void* out, SIP* bytes_read, SIP size) {
  CHECK_RETURN_VAL(size, false)
  FileBuffer* fbuf = m_internal_buffer;
  CHECK_RETURN_VAL(fbuf, false)

  SIP bytes_left = fbuf->len - fbuf->offset;
  SIP total_bytes_read = 0;
  if (!fbuf->is_writing) {
    if (bytes_left >= size) {
      memcpy(out, fbuf->buffer + fbuf->offset, size);
      fbuf->offset += size;
      maybe_assign(bytes_read, size);
      return true;
    } else if (bytes_left) {
      // Copy the rest of the file buffer.
      memcpy(out, fbuf->buffer + fbuf->offset, bytes_left);
      fbuf->offset = fbuf->len;
      size -= bytes_left;
      out = (U8*)out + bytes_left;
      total_bytes_read = bytes_left;
    }
  }
  // Update the file buffer.
  SIP bytes_read_plat;
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    f_read_plat(out, &bytes_read_plat, size);
    total_bytes_read += bytes_read_plat;
    maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  if (!f_read_plat(fbuf->buffer, &bytes_read_plat, gc_max_buffer_size)) {
    maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  fbuf->len = bytes_read_plat;
  fbuf->is_writing = false;
  SIP copy_len = min(bytes_read_plat, size);
  memcpy(out, fbuf->buffer, copy_len);
  fbuf->offset = copy_len;
  total_bytes_read += copy_len;
  maybe_assign(bytes_read, total_bytes_read);
  return true;
}

bool ngFile::f_write(SIP* bytes_written, const void* in, SIP size) {
  CHECK_RETURN_VAL(size, false);
  FileBuffer* fbuf = m_internal_buffer;
  CHECK_RETURN_VAL(fbuf, false);

  SIP bytes_left = fbuf->len - fbuf->offset;
  SIP total_bytes_written = 0;
  if (fbuf->is_writing) {
    if (bytes_left >= size) {
      memcpy(fbuf->buffer + fbuf->offset, in, size);
      fbuf->offset += size;
      maybe_assign(bytes_written, size);
      return true;
    } else if (bytes_left) {
      // Copy to the rest of the file buffer.
      memcpy(fbuf->buffer + fbuf->offset, in, bytes_left);
      fbuf->offset = fbuf->len;
      size -= bytes_left;
      in = (U8*)in + bytes_left;
      SIP bytes_written_plat;
      if (!f_write_plat(&bytes_written_plat, fbuf->buffer, fbuf->offset)) {
        return false;
      }
      total_bytes_written = bytes_left;
    }
  }
  // Update the file buffer.
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    SIP bytes_written_plat;
    bool rv = f_write_plat(&bytes_written_plat, in, size);
    total_bytes_written += bytes_written_plat;
    maybe_assign(bytes_written, total_bytes_written);
    return rv;
  }
  fbuf->len = gc_max_buffer_size;
  fbuf->is_writing = true;
  memcpy(fbuf->buffer, in, size);
  fbuf->offset = size;
  maybe_assign(bytes_written, total_bytes_written);
  return true;
}

void ngFile::f_seek(enum EFileFrom from, SIP distance) {
  FileBuffer* fbuf = m_internal_buffer;
  if (!fbuf) {
    f_seek_plat(from, distance);
    return;
  }

  if (from != EFILE_FROM_CURRENT) {
    if (fbuf->is_writing) {
      f_flush();
    }
    f_seek_plat(from, distance);
    return;
  }
  // from == EFILE_FROM_CURRENT
  SIP bytes_left = fbuf->len - fbuf->offset;
  if (distance >= bytes_left) {
    if (fbuf->is_writing) {
      f_flush();
      // We are seeking from fbuf->offset after flushing.
      f_seek_plat(from, distance);
    } else {
      // We are seeking from the end of the FileBuffer.
      f_seek_plat(from, distance - bytes_left);
    }
    return;
  }

  // Seeking inside the file buffer.
  if (fbuf->is_writing) {
    memset(fbuf->buffer, 0, distance);
  }
  fbuf->offset += distance;
}

void ngFile::f_flush() {
  FileBuffer* fbuf = m_internal_buffer;
  if (fbuf) {
    if (fbuf->is_writing && fbuf->offset > 0) {
      CHECK_RETURN(f_write_plat(NULL, fbuf->buffer, fbuf->offset));
      fbuf->is_writing = false;
      fbuf->offset = 0;
      fbuf->len = 0;
    }
  }
}
