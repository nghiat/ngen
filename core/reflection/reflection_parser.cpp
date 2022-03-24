//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_allocators.h"
#include "core/core_init.h"
#include "core/command_line.h"
#include "core/dynamic_array.inl"
#include "core/file.h"
#include "core/hash_table.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/mono_time.h"
#include "core/path.h"
#include "core/path_utils.h"
#include "core/string.h"
#include "core/string_utils.h"
#include "core/utils.h"

#include "third_party/libclang/include/clang-c/CXCompilationDatabase.h"
#include "third_party/libclang/include/clang-c/Index.h"

#include <string.h>

static char* g_class_name_ = NULL;
static const char* gc_init_fields_field_ = R"(  g_class_info_.m_fields.append("%s");
)";

static char* copy_string(Allocator_t* allocator, const char* str) {
  char* rv = NULL;
  Cstring_t str2(str);
  rv = (char*)allocator->alloc(str2.m_length + 1);
  M_check_return_val(rv, NULL);
  memcpy(rv, str, str2.m_length);
  rv[str2.m_length] = 0;
  return rv;
}

enum CXChildVisitResult get_attr_cursor_(CXCursor cursor, CXCursor parent, CXClientData client_data) {
  if (clang_isAttribute(clang_getCursorKind(cursor))) {
    *(CXCursor*)client_data = cursor;
    return CXChildVisit_Break;
  }
  return CXChildVisit_Continue;
}

bool check_cursor_attribute_(CXCursor cursor, const char* attr_name) {
  bool rv = false;
  if (clang_Cursor_hasAttrs(cursor)) {
    CXCursor attr_cursor = clang_getNullCursor();
    clang_visitChildren(cursor, &get_attr_cursor_, &attr_cursor);
    if (!clang_Cursor_isNull(attr_cursor)) {
      CXString attr_str = clang_getCursorSpelling(attr_cursor);
      M_scope_exit(clang_disposeString(attr_str));
      rv = strcmp(clang_getCString(attr_str), attr_name) == 0;
    }
  }
  return rv;
}

static Dynamic_array_t<Cstring_t> g_fields_;

enum CXChildVisitResult visit_reflected_class_(CXCursor cursor, CXCursor parent, CXClientData client_data) {
  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_CXXMethod || kind == CXCursor_FieldDecl) {
    if (check_cursor_attribute_(cursor, "reflected")) {
      CXString method_name = clang_getCursorSpelling(cursor);
      M_scope_exit(clang_disposeString(method_name));
      if (kind == CXCursor_CXXMethod) {
      } else {
        g_fields_.append(copy_string(g_general_allocator, clang_getCString(method_name)));
      }
    }
    return CXChildVisit_Continue;
  }
  return CXChildVisit_Recurse;
}

Linear_allocator_t<> clang_allocator("clang_allocator");

int main(int argc, char** argv) {
  core_init(M_txt("reflection.log"));

  Command_line_t cl;
  cl.init(g_persistent_allocator);
  M_scope_exit(cl.destroy());
  cl.register_flag(NULL, "--reflection-path", e_value_type_string);
  cl.register_flag(NULL, "--cc-out-dir", e_value_type_string);
  cl.parse(argc, argv);
  Cstring_t reflection_path(cl.get_flag_value("--reflection-path").get_string());
  Cstring_t cc_out_dir(cl.get_flag_value("--cc-out-dir").get_string());

  const Dynamic_array_t<const char*>& unnamed_args = cl.get_unnamed_args();
  M_check(unnamed_args.len() == 1);
  Path_t input_path = Path_t::from_char(unnamed_args[0]);
  Path8_t input_path8 = input_path.get_path8();
  Path_t input_path_parent = input_path.get_parent_dir();
  clang_allocator.init();
  M_scope_exit(clang_allocator.destroy());
  {
    CXIndex index = clang_createIndex(0, 0);
    M_scope_exit(clang_disposeIndex(index));

    Path8_t exe_dir_path8 = g_exe_dir.get_path8();
    CXCompilationDatabase_Error error;
    CXCompilationDatabase compile_db = clang_CompilationDatabase_fromDirectory(exe_dir_path8.m_path, &error);
    M_scope_exit(clang_CompilationDatabase_dispose(compile_db));
    CXCompileCommands commands = clang_CompilationDatabase_getAllCompileCommands(compile_db);
    M_scope_exit(clang_CompileCommands_dispose(commands));
    int command_count = clang_CompileCommands_getSize(commands);
    for (int i = 0; i < command_count; ++i) {
      CXCompileCommand command = clang_CompileCommands_getCommand(commands, i);
      CXString filename = clang_CompileCommand_getFilename(command);
      M_scope_exit(clang_disposeString(filename));
      Path_t filename_path = Path_t::from_char(clang_getCString(filename));
      Path_t filename_path_parent = filename_path.get_parent_dir();
      if (!filename_path_parent.equals(input_path_parent)) {
        continue;
      }
      int arg_count = clang_CompileCommand_getNumArgs(command);
      Dynamic_array_t<const char*> args;
#if M_os_is_win()
      const char* additional_args[] = { "/Tp" };
#else
      const char* additional_args[] = { "-x", "c++" };
#endif
      args.init(&clang_allocator);
      args.reserve(arg_count + static_array_size(additional_args));
      for (int j = 0; j < arg_count; ++j) {
        CXString arg = clang_CompileCommand_getArg(command, j);
        M_scope_exit(clang_disposeString(arg));
        char* arg_cstr = copy_string(&clang_allocator, clang_getCString(arg));
        if (strstr(arg_cstr, "showIncludes")) {
          continue;
        }

        bool should_break = false;
        if (char* dot_p = strstr(arg_cstr, ".cpp")) {
          for (const char* additional_arg : additional_args) {
            args.append(additional_arg);
          }
          arg_cstr = input_path8.m_path;
          should_break = true;
        }
        args.append(arg_cstr);
        if (should_break) {
          break;
        }
      }

      S64 t = mono_time_now();
      CXTranslationUnit unit;
      // TODO: Remember to change the working directory
      CXErrorCode tu_error = clang_parseTranslationUnit2(index, NULL, args.m_p, args.len(), nullptr, 0, CXTranslationUnit_None, &unit);
      M_check(tu_error == CXError_Success);
      M_check(unit);
      M_scope_exit(clang_disposeTranslationUnit(unit));
      int diagnostic_count = clang_getNumDiagnostics(unit);
      bool has_errors = false;
      for (int j = 0; j < diagnostic_count; ++j) {
          CXDiagnostic diagnostic = clang_getDiagnostic(unit, j);
          CXString diagnostic_string = clang_formatDiagnostic(diagnostic,clang_defaultDiagnosticDisplayOptions());
          M_scope_exit(clang_disposeString(diagnostic_string));
          CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diagnostic);
          if (severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal) {
            has_errors = true;
            M_logf("%s", clang_getCString(diagnostic_string));
          }
      }
      M_check_return_val(!has_errors, 1);

      CXCursor tu_cursor = clang_getTranslationUnitCursor(unit);
      clang_visitChildren(tu_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> enum CXChildVisitResult {
        CXCursorKind kind = clang_getCursorKind(cursor);
        CXString cursor_name = clang_getCursorSpelling(cursor);
        const char* cursor_name_cstr = clang_getCString(cursor_name);
        M_scope_exit(clang_disposeString(cursor_name));
        if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl) {
          if (check_cursor_attribute_(cursor, "reflected")) {
            Allocator_t* allocator = (Allocator_t*)client_data;
            g_class_name_ = copy_string(allocator, cursor_name_cstr);
            g_fields_.init(&clang_allocator);
            clang_visitChildren(cursor, &visit_reflected_class_, client_data);
          }
          return CXChildVisit_Continue;
        }
        return CXChildVisit_Recurse;
      }, &clang_allocator);
      Path_t reflection_header_path = Path_t::from_char(cc_out_dir);
      reflection_header_path = reflection_header_path.join(input_path.get_name());
      reflection_header_path.m_path_str = reflection_header_path.m_path_str.get_substr_till(reflection_header_path.m_path_str.find_char_reverse(M_txt('.')));
      reflection_header_path.m_path_str.append(M_txt(".reflection.cpp"));
      File_t f;
      f.open(reflection_header_path.m_path, e_file_mode_write);
      M_scope_exit(f.close());
      Cstring_t path_without_dot_dot;
      for (int j = 0; j < input_path8.m_path_str.m_length; ++j) {
        char c = input_path8.m_path_str.m_p[j];
        if (c != M_txt('.') && c != M_txt('\\') && c != M_txt('/')) {
          path_without_dot_dot = input_path8.m_path_str.get_substr_from_offset(j).to_const();
          break;
        }
      }

      Path_t template_path = g_exe_dir.join(M_txt("template")).join(M_txt("template.reflection.cpp"));
      Dynamic_array_t<U8> template_format = File_t::read_whole_file_as_text(&clang_allocator, template_path.m_path);

      auto dict = string_format_setup<char>(&clang_allocator, (char*)template_format.m_p, NULL);
      dict["source_header_path"] = path_without_dot_dot.m_p;
      dict["class_name"] = g_class_name_;
      const int c_field_count_buffer_size = 10;
      char field_count_str[c_field_count_buffer_size];
      M_check(snprintf(NULL, 0, "%lld", g_fields_.len()) < c_field_count_buffer_size - 1);
      snprintf(field_count_str, c_field_count_buffer_size, "%lld", g_fields_.len());
      dict["field_count"] = field_count_str;

      Dynamic_array_t<char> init_fields;
      init_fields.init(&clang_allocator);
      init_fields.append(0);
      for (int j = 0; j < g_fields_.len(); ++j) {
        int field_str_len = snprintf(NULL, 0, gc_init_fields_field_, g_fields_[j].m_p);
        int init_fields_old_len = init_fields.len();
        init_fields.resize(init_fields_old_len + field_str_len);
        // | - 1| to exclude null character
        snprintf(init_fields.m_p + init_fields_old_len - 1, field_str_len + 1, gc_init_fields_field_, g_fields_[j].m_p);
      }
      dict["fields"] = init_fields.m_p;
      M_logi("parse time: %f", mono_time_to_ms(mono_time_now() - t));
      auto final_template = string_format<char>(&clang_allocator, (char*)template_format.m_p, dict);
      f.write(NULL, final_template.m_p, final_template.m_length);
      break;
    }
  }

  return 0;
}
