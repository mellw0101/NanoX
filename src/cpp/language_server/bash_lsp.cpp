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

void get_env_path_binaries(void) {
  if (!env_path_task_running) {
    char **files, **dirs;
    Ulong nfiles,  ndirs;
    if (entries_in_dir("/usr/bin", &files, &nfiles, &dirs, &ndirs) == 0) {
      free_chararray(files, nfiles);
      free_chararray(dirs, ndirs);
    }
    // env_path_task_running = TRUE;
    // auto task = [](void *) -> void * {
    //   char **files, **dirs;
    //   Ulong nfiles,  ndirs;
    //   if (entries_in_dir("/usr/bin", &files, &nfiles, &dirs, &ndirs) == 0) {
    //     free_chararray(files, nfiles);
    //     for (Uint i = 0; i < ndirs; ++i) {
    //       NLOG("Dir: %s\n", dirs[i]);
    //     }
    //     free_chararray(dirs, ndirs);
    //   }
    //   return NULL;
    // };
    // auto callback = [](void *) {
    //   env_path_task_running = FALSE;
    // };
    // submit_task(task, NULL, NULL, callback);
  }
}