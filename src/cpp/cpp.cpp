#include "../include/prototypes.h"

#include <Mlib/def.h>

bool
isCppSyntaxChar(const s8 c)
{
    return (c == '<' || c == '>' || c == '&' || c == '*' || c == '=' || c == '+' || c == '-' || c == '/' || c == '%' ||
            c == '!' || c == '^' || c == '|' || c == '~' || c == '{' || c == '}' || c == '[' || c == ']' || c == '(' ||
            c == ')' || c == ';' || c == ':' || c == ',' || c == '.' || c == '?');
}
