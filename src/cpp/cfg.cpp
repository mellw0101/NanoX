#include "../include/prototypes.h"

#define CFG_FILE_EXT    ".nxcfg"
#define CFG_FOLDER_PATH ".config/nanox/"

static char *cfg_dir = NULL;

/// Init NanoX config.
void init_cfg(void) {
  get_homedir();
  if (!homedir) {
    return;
  }
  cfg_dir = concatenate_path(homedir, CFG_FOLDER_PATH);
  if (!is_dir(cfg_dir)) {
    mkdir(cfg_dir, 0755);
  }
  NLOG("cfg_dir: %s\n", cfg_dir);
}

void cleanup_cfg(void) {
  free(cfg_dir);
}