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

void
netlog_bracket_entry(const bracket_entry &be)
{
    NETLOGGER.log("indent: %lu, lineno: %lu, start: %s.\n", be.indent, be.lineno, be.start ? "TRUE" : "FALSE");
}

void
netlog_func_info(function_info_t *info)
{
    NLOG("  Full func: %s\n"
         "      Start: %i\n"
         "        End: %i\n"
         "  Func name: %s\n"
         "Return type: %s\n"
         "  Param num: %i\n",
         info->full_function, info->start_bracket, info->end_braket, info->name, info->return_type,
         info->number_of_params);
    for (int i = 0; i < info->number_of_params; i++)
    {
        NLOG("    Param %i: %s\n", i + 1, info->params[i]);
    }
}

/* clang-format off */
void 
netlog_lsp_function_decl(function_decl *decl) noexcept
{
    NLOG("file: %s\n"
         "start: %d:%d:%d\n"
         "end: %d:%d:%d\n"
         "name: %s\n"
         "return type: %s\n",
         decl->file.c_str(),
         decl->start.line, decl->start.column, decl->start.offset,
         decl->end.line, decl->end.column, decl->end.offset,
         decl->name.c_str(),
         decl->return_type.c_str());
}
/* clang-format on */