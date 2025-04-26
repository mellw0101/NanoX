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
  directory_t dir;
  if (!env_path_task_running) {
    Ulong npaths;
    char **paths = get_env_paths(&npaths);
    for (Ulong i=0; i<npaths; ++i) {
      directory_data_init(&dir);
      if (directory_get_recurse(paths[i], &dir) != -1) {
        DIRECTORY_ITER(dir, j, entry,
          test_map[entry->name] = {FG_VS_CODE_YELLOW};
        );
      }
      directory_data_free(&dir);
    }
    free_chararray(paths, npaths);
  }
}
