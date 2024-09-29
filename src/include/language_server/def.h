#pragma once

#include "../definitions.h"
/* clang-format off */

struct define_entry_t
{
    string name;
    string value;
};

typedef struct DefineEntry
{
  char *name      = nullptr;
  char *full_decl = nullptr;
  char *value     = nullptr;
  char *file      = nullptr;
  int decl_start_line = -1; 
  int decl_end_line   = -1;

  void delete_data(void) noexcept {
    name      ? free(name)      : (void)0;
    full_decl ? free(full_decl) : (void)0;
    value     ? free(value)     : (void)0;
    file      ? free(file)      : (void)0;
    name      = nullptr;
    full_decl = nullptr;
    value     = nullptr;
    decl_start_line = -1;
    decl_end_line   = -1;
  }

  static DefineEntry create(char *name, char *value) noexcept {
      return {name, value};
  }
}
DefineEntry;

typedef struct IndexFile
{
  char *file;
  linestruct *head;
}
IndexFile;

typedef struct TypeDefEnumEntry {
  char *type  = nullptr;
  char *name  = nullptr;
  char *alias = nullptr;
  char *value = nullptr;
  int decl_line      = -1;
  int type_decl_line = -1;

  __inline__ void delete_data(void) noexcept {
    type  ? free(type)  : (void)0;
    name  ? free(name)  : (void)0;
    alias ? free(alias) : (void)0;
    value ? free(value) : (void)0;
    decl_line      = -1;
    type_decl_line = -1;
  }
} TypeDefEnumEntry;

typedef struct Index
{
  MVector<IndexFile> include;
  MVector<DefineEntry> defines;

  MVector<string> rawtypedef;
  MVector<string> rawenum;
  MVector<string> rawstruct;
  MVector<string> rawclass;

  void delete_data(void) noexcept;
}
Index;

__inline__ void Index::delete_data(void) noexcept {
  for (auto &it : include) {
    while (it.head) {
      linestruct *node = it.head;
      it.head = it.head->next;
      free(node->data);
      free(node);
    }
    free(it.file);
  }
  include.resize(0);
  for (auto &it : defines) {
    it.delete_data();
  }
  defines.resize(0);
}
/* clang-format on */
