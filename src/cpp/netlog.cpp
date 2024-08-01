#include "../include/prototypes.h"

void
netlog_syntaxtype(syntaxtype *s)
{
    NETLOGGER.log("    name: %s\n"
                  "filename: %s\n"
                  "  lineno: %lu\n",
                  s->name, s->filename, s->lineno);
}

/* Log a 'colortype' struct using 'NETLOGGER'. */
void
netlog_colortype(colortype *c)
{
    NETLOGGER.log("        id: %hi\n"
                  "        fg: %hi\n"
                  "        bg: %hi\n"
                  "   pairnum: %hi\n"
                  "attributes: %i\n",
                  c->id, c->fg, c->bg, c->pairnum, c->attributes);
}
