#include "sample/quake_console.h"

#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/linear_allocator.inl"
#include "core/path_utils.h"

struct Line_info_t_ {
  U32 index_in_buffer;
  U32 length;
};

bool Quake_console_t::init(F32 width, F32 height) {
  m_codepoint_allocator.init();
  m_codepoint_buffer.init(&m_codepoint_allocator);
  m_line_indices_allocator.init();
  m_codepoint_line_indices_buffer.init(&m_line_indices_allocator);
  m_f_allocator.init();
  Os_char shader_path[M_max_path_len];
  path_from_exe_dir(shader_path, M_txt("assets/shadow.hlsl"), M_max_path_len);
  m_f_buf = File_t::read_whole_file_as_text(&m_f_allocator, shader_path);

  m_width = width;
  m_height = height;
  m_rect[0] = {0.0f, 0.0f};
  m_rect[1] = {0.0f, m_height};
  m_rect[2] = {(F32)m_width, 0.0f};
  m_rect[3] = {(F32)m_width, 0.0f};
  m_rect[4] = {0.0f, m_height};
  m_rect[5] = {(F32)m_width, m_height};
  return true;
}

void Quake_console_t::destroy() {
  m_codepoint_allocator.destroy();
  m_line_indices_allocator.destroy();
}

void Quake_console_t::append_codepoint(U32 cp) {
  m_codepoint_buffer.append(cp);
  if (cp == (U32)'\n') {
    // g_console_instance_.codepoint_line_indices_buffer.append(g_console_instance_.codepoint_buffer.len() - 1);
  }
}

void Quake_console_t::append_string(const char* str) {
  // TODO UTF-8
  Sip current_len = m_codepoint_buffer.len();
  Sip str_len = strlen(str);
  m_codepoint_buffer.resize(current_len + str_len);
  for (Sip i = 0; i < str_len; ++i) {
    m_codepoint_buffer[current_len + i] = str[i];
  }
  const char* str_end = str + str_len;
  const char* line_ending = str;
  while ((line_ending = (const char*)memchr(line_ending, '\n', str_end - line_ending)) != NULL) {
    // g_console_instance_.codepoint_line_indices_buffer.append((U32)(current_len + (line_ending - str)));
  }
}
