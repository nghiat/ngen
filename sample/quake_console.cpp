#include "sample/quake_console.h"

#include "core/dynamic_array.inl"
#include "core/linear_allocator.inl"

struct Line_info_ {
  U32 index_in_buffer;
  U32 length;
};

struct Quake_console_ {
  Linear_allocator<> codepoint_allocator = Linear_allocator<>("console_codepoint_allocator");
  Dynamic_array<U32> codepoint_buffer;
  Linear_allocator<> line_indices_allocator = Linear_allocator<>("console_line_indices_allocator");
  Dynamic_array<Line_info_> codepoint_line_indices_buffer;
};

static Quake_console_ g_console_instance_;

bool qc_init() {
  g_console_instance_.codepoint_allocator.la_init();
  g_console_instance_.codepoint_buffer.da_init(&g_console_instance_.codepoint_allocator);
  g_console_instance_.line_indices_allocator.la_init();
  g_console_instance_.codepoint_line_indices_buffer.da_init(&g_console_instance_.line_indices_allocator);
  return true;
}

void qc_destroy() {
  g_console_instance_.codepoint_allocator.al_destroy();
  g_console_instance_.line_indices_allocator.al_destroy();
}

void qc_append_codepoint(U32 cp) {
  g_console_instance_.codepoint_buffer.da_append(cp);
  if (cp == (U32)'\n') {
    // g_console_instance_.codepoint_line_indices_buffer.da_append(g_console_instance_.codepoint_buffer.da_len() - 1);
  }
}

void qc_append_string(const char* str) {
  // TODO UTF-8
  Sip current_len = g_console_instance_.codepoint_buffer.da_len();
  Sip str_len = strlen(str);
  g_console_instance_.codepoint_buffer.da_resize(current_len + str_len);
  for (Sip i = 0; i < str_len; ++i) {
    g_console_instance_.codepoint_buffer[current_len + i] = str[i];
  }
  const char* str_end = str + str_len;
  const char* line_ending = str;
  while ((line_ending = (const char*)memchr(line_ending, '\n', str_end - line_ending)) != NULL) {
    // g_console_instance_.codepoint_line_indices_buffer.da_append((U32)(current_len + (line_ending - str)));
  }
}
