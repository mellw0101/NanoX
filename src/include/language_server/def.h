#pragma once

#include "../definitions.h"
/* clang-format off */

/* Identier for casting data. */
typedef enum class SynxType {
  DEFINE_ENTRY
} SynxType;
/* Struct to hold the type and ptr to the data. */
typedef struct {
  SynxType type; /* Data type to be cast into. */
  void    *data; /* Ptr to the actual data. */
} SynxMapEntry;
/* Custom 'char *' hash struct, for the synx map. */
typedef struct CharPtrHash {
  /* Return`s hash based on '__ptr' content. */
  Ulong operator () (const char *__ptr) const {
    return hash<string>()(__ptr);
  }
} CharPtrHash;
/* Custom 'char *' equal struct, for the synx map. */
typedef struct CharPtrEqual {
  bool operator () (const char *__lhs, const char *__rhs) const {
    return strcmp(__lhs, __rhs);
  }
} CharPtrEqual;
extern unordered_map<char *, SynxMapEntry, CharPtrHash, CharPtrEqual> synx_map;

typedef struct DefineEntry {
  char *name;
  string full_decl;
  string value;
  string file;
  int decl_start_line = -1; 
  int decl_end_line   = -1;
} DefineEntry;

typedef struct IndexFile {
 private:
  Ulong checksum;
  char *filename;
  linestruct *filetop;
  linestruct *filebot;

  int  open_file(FILE **f);
  void read_lines(FILE *f, int fd);
  void calc_checksum(void) noexcept;

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
  MVector<var_t> variabels;
  MVector<ClassData> classes;

  MVector<string> rawtypedef;
  MVector<string> rawenum;
  MVector<string> rawstruct;

  void delete_data(void) noexcept {
    for (auto &[filename, file] : include) {
      file.delete_data();
    }
    include.clear();
    for (auto &[name, de] : defines) {
      free(de.name);
    }
    defines.clear();
    for (auto &it : classes) {
      it.delete_data();
    }
    classes.resize(0);
  }
} Index;

/* clang-format on */
