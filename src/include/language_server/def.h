#pragma once

#include "../definitions.h"
/* clang-format off */

struct define_entry_t
{
    string name;
    string value;
};

typedef struct DefineEntry
{
    char *name      = nullptr;
    char *full_decl = nullptr;
    char *value     = nullptr;
    
    void delete_data(void) noexcept {
        name      ? free(name)      : (void)0;
        full_decl ? free(full_decl) : (void)0;
        value     ? free(value)     : (void)0;
    }

    void delete_data(void) const noexcept {
        name      ? free(name)      : (void)0;
        full_decl ? free(full_decl) : (void)0;
        value     ? free(value)     : (void)0;
    }

    static DefineEntry create(char *name, char *value) noexcept {
        return {name, value};
    }
}
DefineEntry;

typedef struct IndexFile
{
    char *file;
    linestruct *head;
}
IndexFile;

typedef struct Index
{
    MVector<IndexFile> include;
    MVector<DefineEntry> defines;

    void delete_data(void) noexcept;
}
Index;

inline void Index::delete_data(void) noexcept
{
    for (auto &it : include)
    {
        while (it.head)
        {
            linestruct *node = it.head;
            it.head = it.head->next;
            free(node->data);
            free(node);
        }
        free(it.file);
    }
    include.resize(0);
}
/* clang-format on */
