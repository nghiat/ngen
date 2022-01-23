//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/debug.h"

#include "core/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <elf.h>
#include <execinfo.h>
#include <fcntl.h>
#include <link.h> // For ElfW macro.
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define M_max_sections_ 64
#define M_max_symbols_ 2048
#define M_max_symbol_name_length_ 256

static void find_symbol_name(const char* path, size_t offset, char* out_symbol_name) {
  out_symbol_name[0] = '\0';
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    return;
  }
  ElfW(Ehdr) ehdr;
  pread(fd, &ehdr, sizeof(ehdr), 0);
  ElfW(Shdr) sections[M_max_sections_];
  int remaining_sections = ehdr.e_shnum;
  int read_sections = 0;
  while (remaining_sections) {
    int section_count = remaining_sections > M_max_sections_ ? M_max_sections_ : remaining_sections;
    pread(fd, &sections, section_count * ehdr.e_shentsize, ehdr.e_shoff + read_sections * ehdr.e_shentsize);
    remaining_sections -= section_count;
    read_sections += section_count;
    for (int i = 0; i < section_count; ++i) {
      if (sections[i].sh_type == SHT_SYMTAB) {
        ElfW(Shdr)* symtab = &sections[i];
        int remaining_symbols = symtab->sh_size / symtab->sh_entsize;
        int read_symbols = 0;
        while (remaining_symbols) {
          int symbol_count =
              remaining_symbols > M_max_symbols_ ? M_max_symbols_ : remaining_symbols;
          ElfW(Sym) symbols[M_max_symbols_];
          pread(fd, &symbols, symbol_count * sizeof(ElfW(Sym)), symtab->sh_offset + read_symbols * sizeof(ElfW(Sym)));
          remaining_symbols -= symbol_count;
          read_symbols += symbol_count;
          for (int j = 0; j < symbol_count; ++j) {
            ElfW(Sym)* sym = &symbols[j];
            if (sym->st_name && sym->st_size && sym->st_value <= offset && sym->st_value + sym->st_size > offset) {
              ElfW(Shdr) strtab;
              // symtab->sh_link is the index of the related string table header
              // in the section header table.
              pread(fd, &strtab, sizeof(strtab), ehdr.e_shoff + symtab->sh_link * ehdr.e_shentsize);
              char symbol_name[M_max_symbol_name_length_];
              pread(fd, &symbol_name, M_max_symbol_name_length_, strtab.sh_offset + sym->st_name);
              memcpy(out_symbol_name, symbol_name, M_max_symbol_name_length_);
              goto out;
            }
          }
        }
      }
    }
  }
out:
  close(fd);
}

bool debug_init() {
  return true;
}

void debug_get_stack_trace(char* buffer, int len) {
  memset(buffer, 0, len);
  M_check_return(len <= M_max_stack_trace_length_);
  void* traces[M_max_traces_];
  int count = backtrace((void**)traces, M_max_traces_);
  char** symbols = backtrace_symbols(traces, count);
  if (!symbols) {
    return;
  }
  size_t buf_remaning_size = len - 1;
  for (int i = 0; i < count; ++i) {
    char* symbol = symbols[i];
    char path[PATH_MAX];
    // symbol looks like this:
    // /usr/lib/libc.so.6(__libc_start_main+0xf3) [0x7f7d878f7023]

    // find '('
    const char* end_of_path = (const char*)memchr((void*)symbol, '(', strlen(symbol));
    size_t path_len = end_of_path - symbol;
    memcpy(path, symbol, path_len);
    path[path_len] = '\0';
    // Check if the symbol name exists.
    char symbol_name[M_max_symbol_name_length_];
    symbol_name[0] = '\0';
    if (end_of_path[1] != ')') {
      if (end_of_path[1] != '+') {
        const char* end_of_symbol_name = (const char*)memchr((void*)end_of_path, '+', strlen(end_of_path));
        int symbol_len = end_of_symbol_name - end_of_path - 1;
        memcpy(&symbol_name, end_of_path + 1, symbol_len);
        symbol_name[symbol_len] = '\0';
      } else {
        const char* start_of_offset = (const char*)memchr((void*)end_of_path, 'x', strlen(end_of_path));
        size_t offset; sscanf(start_of_offset + 1, "%lx", &offset); find_symbol_name(path, offset, symbol_name);
      }
    }
    int written = snprintf(buffer, buf_remaning_size, "%s: %s\n", path, symbol_name);
    buffer += written;
    buf_remaning_size -= written;
    if (!buf_remaning_size) {
      break;
    }
  }
  free(symbols);
}

bool debug_is_debugger_attached() {
  int fd = open("/proc/self/status", O_RDONLY);
  if (fd == -1) {
    return false;
  }
  const char* tracerpid_str = "TracerPid:\t";
  // TracerPid is usually in the first few lines, so 512 is way more than enough.
  char buf[512];
  int len = read(fd, buf, 512);
  buf[len - 1] = '\0';
  const char* p = strstr(buf, tracerpid_str);
  bool rv = false;
  if (p) {
    p += strlen(tracerpid_str);
    if (*p != '0' || *(p + 1) != '\n') {
      rv = true;
    }
  }
  close(fd);
  return rv;
}
