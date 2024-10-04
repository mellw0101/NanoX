#pragma once

#include "../definitions.h"

using std::unordered_map;

/* clang-format off */

struct define_entry_t {
  string name;
  string value;
};

typedef struct DefineEntry {
  string name;
  string full_decl;
  string value;
  string file;
  int decl_start_line = -1; 
  int decl_end_line   = -1;
} DefineEntry;

typedef struct IndexFile {
  char *file;
  linestruct *head = nullptr;
} IndexFile;

typedef struct ClassData {
  string raw_data;
  char *name = nullptr;
  
  __inline__ void delete_data(void) noexcept {
    name ? free(name) : (void)0;
    name = nullptr;
  }
} ClassData;

typedef struct Index {
  unordered_map<string, IndexFile> include;
  unordered_map<string, DefineEntry> defines;
  MVector<var_t> variabels;
  MVector<ClassData> classes;

  MVector<string> rawtypedef;
  MVector<string> rawenum;
  MVector<string> rawstruct;

  void delete_data(void) noexcept {
    for (auto &[file, data] : include) {
      while (data.head) {
        linestruct *node = data.head;
        data.head = data.head->next;
        free(node->data);
        free(node);
      }
      free(data.file);
    }
    include.clear();
    defines.clear();
    for (auto &it : classes) {
      it.delete_data();
    }
    classes.resize(0);
  }
} Index;

/* clang-format on */
