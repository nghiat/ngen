//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2022             //
//----------------------------------------------------------------------------//

#include "core/core_init.h"
#include "core/dynamic_array.inl"
#include "core/linear_allocator.inl"
#include "core/log.h"
#include "core/string.inl"
#include "core/utils.h"

#include "third_party/libclang/include/clang-c/CXCompilationDatabase.h"
#include "third_party/libclang/include/clang-c/Index.h"

#include <string.h>

#define R_class __attribute__((annotate("reflected")))
#define R_field __attribute__((annotate("reflected")))
#define R_method __attribute__((annotate("reflected")))

class R_class Reflected_class_t_ {
public:
  R_method
  void reflected_method() {}

  R_field
  int reflected_field;
};

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

enum CXChildVisitResult visit_reflected_class_(CXCursor cursor, CXCursor parent, CXClientData client_data) {
  CXCursorKind kind = clang_getCursorKind(cursor);
  if (kind == CXCursor_CXXMethod || kind == CXCursor_FieldDecl) {
    if (check_cursor_attribute_(cursor, "reflected")) {
      CXString method_name = clang_getCursorSpelling(cursor);
      M_scope_exit(clang_disposeString(method_name));
      if (kind == CXCursor_CXXMethod) {
        M_logi("  method %s", clang_getCString(method_name));
      } else {
        M_logi("  field %s", clang_getCString(method_name));
      }
    }
    return CXChildVisit_Continue;
  }
  return CXChildVisit_Recurse;
}

int main() {
  core_init(M_os_txt("reflection.log"));
  Linear_allocator_t<> clang_allocator("clang_allocator");
  clang_allocator.init();
  M_scope_exit(clang_allocator.destroy());
  {
    CXIndex index = clang_createIndex(0, 0);
    M_scope_exit(clang_disposeIndex(index));

    CXCompilationDatabase_Error error;
    CXCompilationDatabase compile_db = clang_CompilationDatabase_fromDirectory("D:\\projects\\ngen\\out\\debug", &error);
    M_scope_exit(clang_CompilationDatabase_dispose(compile_db));
    CXCompileCommands commands = clang_CompilationDatabase_getAllCompileCommands(compile_db);
    M_scope_exit(clang_CompileCommands_dispose(commands));
    int command_count = clang_CompileCommands_getSize(commands);
    for (int i = 0; i < command_count; ++i) {
      CXCompileCommand command = clang_CompileCommands_getCommand(commands, i);
      CXString filename = clang_CompileCommand_getFilename(command);
      M_scope_exit(clang_disposeString(filename));
      if (!strstr(clang_getCString(filename), "reflection.cpp")) {
        continue;
      }
      int arg_count = clang_CompileCommand_getNumArgs(command);
      Dynamic_array_t<const char*> args;
      args.init(&clang_allocator);
      args.reserve(arg_count);
      for (int j = 0; j < arg_count; ++j) {
        CXString arg = clang_CompileCommand_getArg(command, j);
        const char* arg_cstr = clang_getCString(arg);
        if (strstr(arg_cstr, "showIncludes")) {
          continue;
        }
        args.append(arg_cstr);
      }
      // TODO Cleanup args
      // M_scope_exit_lambda([&]() {
      //   for (int j = 0; j < arg_count; ++j) {
      //     CXString arg = clang_CompileCommand_getArg(command, j);
      //     clang_disposeString(arg);
      //   }
      // });

      CXTranslationUnit unit;
      // TODO: Remember to change the working directory
      CXErrorCode tu_error = clang_parseTranslationUnit2(index, NULL, args.m_p, arg_count, nullptr, 0, CXTranslationUnit_None, &unit);
      M_check(tu_error == CXError_Success);
      M_check(unit);
      M_scope_exit(clang_disposeTranslationUnit(unit));
      // int diagnostic_count = clang_getNumDiagnostics(unit);
      // for (int j = 0; j < diagnostic_count; ++j) {
      //     CXDiagnostic diagnostic = clang_getDiagnostic(unit, j);
      //     CXString diagnostic_string = clang_formatDiagnostic(diagnostic,clang_defaultDiagnosticDisplayOptions());
      //     M_scope_exit(clang_disposeString(diagnostic_string));
      //     M_logi("%s", clang_getCString(diagnostic_string));
      // }

      CXCursor tu_cursor = clang_getTranslationUnitCursor(unit);
      clang_visitChildren(tu_cursor, [](CXCursor cursor, CXCursor parent, CXClientData client_data) -> enum CXChildVisitResult {
        CXCursorKind kind = clang_getCursorKind(cursor);
        if (kind == CXCursor_StructDecl || kind == CXCursor_ClassDecl) {
          if (check_cursor_attribute_(cursor, "reflected")) {
            CXString class_name = clang_getCursorSpelling(cursor);
            M_scope_exit(clang_disposeString(class_name));
            M_logi("class %s", clang_getCString(class_name));
            clang_visitChildren(cursor, &visit_reflected_class_, NULL);
          }
          return CXChildVisit_Continue;
        }
        return CXChildVisit_Recurse;
      }, NULL);
    }


  }

  return 0;
}
