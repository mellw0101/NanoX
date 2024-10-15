#pragma once

#include "../definitions.h"
/* clang-format off */

typedef struct DefineEntry {
  char *name;
  string full_decl;
  string value;
  string file;
  int decl_start_line = -1; 
  int decl_end_line   = -1;
} DefineEntry;

typedef struct EnumEntry {
  char *name;
} EnumEntry;

typedef struct TypedefStruct {
  char *name;
  char *alias;
} TypedefStruct;

typedef struct StructEntry {
  char *name;
  Ulong decl_st;
  Ulong decl_end;
  char *filename;
} StructEntry;

typedef struct FunctionDef {
  char *file;
  char *name;
  Uint decl_st;
  Uint decl_end;
} FunctionDef;

typedef struct VarDecl {
  char *file;
  char *type;
  char *name;
  char *value;
  Uint decl_st;
  Uint decl_end;
} VarDecl;

typedef struct IndexFile {
 private:
  time_t last_time_changed;
  char *filename;
  linestruct *filetop;
  linestruct *filebot;

  int  open_file(FILE **f);
  void read_lines(FILE *f, int fd);
  void get_last_time_changed(void);

 public:
  const char *const &name(void) const noexcept;
  linestruct *const &top(void) const noexcept;

  bool has_changed(void) noexcept;
  void delete_data(void) noexcept;
  void read_file(const char *path);

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
  unordered_map<string, EnumEntry> enums;
  unordered_map<string, TypedefStruct> tdstructs;
  unordered_map<string, StructEntry> structs;
  unordered_map<string, FunctionDef> functiondefs;
  unordered_map<string, MVector<VarDecl>> vars;

  MVector<var_t> variabels;
  MVector<ClassData> classes;

  MVector<string> rawtypedef;
  MVector<string> rawenum;
  MVector<string> rawstruct;

  void delete_data(void) noexcept {
    /* Clear all includes. */
    for (auto &[filename, file] : include) {
      file.delete_data();
    }
    include.clear();
    /* Clear all defines. */
    for (auto &[name, de] : defines) {
      free(de.name);
    }
    defines.clear();
    /* Clear all enums. */
    for (auto &[name, e] : enums) {
      free(e.name);
    }
    enums.clear();
    /* Clear all typedef structs. */
    for (auto &[alias, tds] : tdstructs) {
      free(tds.name);
      free(tds.alias);
    }
    tdstructs.clear();
    /* Clear all structs. */
    for (auto &[name, st] : structs) {
      free(st.filename);
      free(st.name);
    }
    structs.clear();
    /* Clear all function defenitions. */
    for (auto &[name, fd] : functiondefs) {
      free(fd.name);
      free(fd.file);
    }
    functiondefs.clear();
    /* Clear all variabels. */
    for (auto &[name, var_vec] : vars) {
      for (auto &v : var_vec) {
        free(v.name);
        free(v.file);
        free(v.type);
      }
      var_vec.resize(0);
    }
    vars.clear();
    for (auto &it : classes) {
      it.delete_data();
    }
    classes.resize(0);
  }
} Index;

/* clang-format on */
