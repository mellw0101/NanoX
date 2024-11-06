#include "../../include/prototypes.h"

#define DEFAULT_CFG_FILE ".nxcfg"

void init_cfg_file(void) {
  get_homedir();
  if (!homedir) {
    return;
  }
  unix_socket_debug("homedir: %s\n", homedir);
}