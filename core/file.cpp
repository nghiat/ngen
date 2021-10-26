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
  Sz len = 0;
  // offset from the start of the buffer.
  Sz offset = 0;
  bool is_in_used : 1;
  bool is_writing : 1;
};

static FileBuffer g_buffers[gc_max_buffer];

bool File::f_init() {
  for (int i = 0; i < gc_max_buffer; ++i) {
    g_buffers[i].buffer = (U8*)g_persistent_allocator->al_alloc(gc_max_buffer_size);
    g_buffers[i].is_in_used = false;
    if (!g_buffers[i].buffer) {
      return false;
    }
  }

  return true;
}

Dynamic_array<U8> File::f_read_whole_file_as_text(Allocator* allocator, const Os_char* path) {
  Dynamic_array<U8> buffer;
  File f;
  f.f_open(path, e_file_mode_read);
  M_check_return_val(f.f_is_valid(), buffer);
  Sip file_size = f.f_get_size();
  buffer.da_init(allocator);
  buffer.da_resize(file_size + 1);
  f.f_read_plat_(&buffer[0], NULL, file_size);
  buffer[file_size] = 0;
  f.f_close();
  return buffer;
}

bool File::f_open(const Os_char* path, enum E_file_mod mode) {
  bool rv = f_open_plat_(path, mode);
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
  M_check_log_return_val(m_internal_buffer, false, "Can't get a file buffer, too many files are being opened");
  return true;
}

void File::f_close() {
  f_flush();
  f_close_plat_();
  if (m_internal_buffer) {
    for (int i = 0; i < gc_max_buffer; ++i) {
      if (m_internal_buffer == &g_buffers[i]) {
        g_buffers[i].is_in_used = false;
        m_internal_buffer = NULL;
        break;
      }
    }
  }
  M_check_return(!m_internal_buffer);
}

bool File::f_read(void* out, Sip* bytes_read, Sip size) {
  M_check_return_val(size, false)
  FileBuffer* fbuf = m_internal_buffer;
  M_check_return_val(fbuf, false)

  Sip bytes_left = fbuf->len - fbuf->offset;
  Sip total_bytes_read = 0;
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
  Sip bytes_read_plat;
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    f_read_plat_(out, &bytes_read_plat, size);
    total_bytes_read += bytes_read_plat;
    maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  if (!f_read_plat_(fbuf->buffer, &bytes_read_plat, gc_max_buffer_size)) {
    maybe_assign(bytes_read, total_bytes_read);
    return total_bytes_read != 0;
  }
  fbuf->len = bytes_read_plat;
  fbuf->is_writing = false;
  Sip copy_len = min(bytes_read_plat, size);
  memcpy(out, fbuf->buffer, copy_len);
  fbuf->offset = copy_len;
  total_bytes_read += copy_len;
  maybe_assign(bytes_read, total_bytes_read);
  return true;
}

bool File::f_write(Sip* bytes_written, const void* in, Sip size) {
  M_check_return_val(size, false);
  FileBuffer* fbuf = m_internal_buffer;
  M_check_return_val(fbuf, false);

  Sip bytes_left = fbuf->len - fbuf->offset;
  Sip total_bytes_written = 0;
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
      Sip bytes_written_plat;
      if (!f_write_plat_(&bytes_written_plat, fbuf->buffer, fbuf->offset)) {
        return false;
      }
      total_bytes_written = bytes_left;
    }
  }
  // Update the file buffer.
  if (size >= gc_max_buffer_size) {
    // Too big for a file buffer, just read straight into the out buffer.
    Sip bytes_written_plat;
    bool rv = f_write_plat_(&bytes_written_plat, in, size);
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

void File::f_seek(enum E_file_from from, Sip distance) {
  FileBuffer* fbuf = m_internal_buffer;
  if (!fbuf) {
    f_seek_plat_(from, distance);
    return;
  }

  if (from != e_file_from_current) {
    if (fbuf->is_writing) {
      f_flush();
    }
    f_seek_plat_(from, distance);
    return;
  }
  // from == e_file_from_current
  Sip bytes_left = fbuf->len - fbuf->offset;
  if (distance >= bytes_left) {
    if (fbuf->is_writing) {
      f_flush();
      // We are seeking from fbuf->offset after flushing.
      f_seek_plat_(from, distance);
    } else {
      // We are seeking from the end of the FileBuffer.
      f_seek_plat_(from, distance - bytes_left);
    }
    return;
  }

  // Seeking inside the file buffer.
  if (fbuf->is_writing) {
    memset(fbuf->buffer, 0, distance);
  }
  fbuf->offset += distance;
}

void File::f_flush() {
  FileBuffer* fbuf = m_internal_buffer;
  if (fbuf) {
    if (fbuf->is_writing && fbuf->offset > 0) {
      M_check_return(f_write_plat_(NULL, fbuf->buffer, fbuf->offset));
      fbuf->is_writing = false;
      fbuf->offset = 0;
      fbuf->len = 0;
    }
  }
}
