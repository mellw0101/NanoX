#include "../include/prototypes.h"

void
create_bracket_entry(unsigned long lineno, unsigned long indent, bool is_start)
{
    bracket_entry be {lineno, indent, is_start};
    bracket_entrys.push_back(be);
}
