#include "../../include/prototypes.h"

blsp *blsp::_instance = NULL;

static bool env_path_task_running = false;

blsp *const &blsp::instance(void) {
  if (!_instance) {
    _instance = new blsp();
    atexit([]() {
      delete _instance;
    });
  }
  return _instance;
}

/* Get all executables in all PATH env var parts. */
void get_env_path_binaries(void) {
  if (!env_path_task_running) {
    Ulong npaths;
    char **paths = get_env_paths(&npaths);
    for (Ulong i = 0; i < npaths; ++i) {
      char **files, **dirs;
      Ulong  nfiles,  ndirs;
      if (get_all_entries_in_dir(paths[i], &files, &nfiles, &dirs, &ndirs) != 0) {
        logE("Failed to get entries in dir: '%s'.\n", paths[i]);
        continue;
      }
      for (Ulong i = 0; i < nfiles; ++i) {
        test_map[files[i]] = {FG_VS_CODE_YELLOW};
      }
      free_chararray(files, nfiles);
      free_chararray(dirs, ndirs);
    }
    free_chararray(paths, npaths);
  }
}