#include "../include/c_prototypes.h"


#ifndef NODIR
#    if defined(_POSIX_VERSION) /* Might be defined in unistd.h.  */
/* POSIX does not require that the d_ino field be present, and some
 * systems do not provide it. */
#        define REAL_DIR_ENTRY(dp) 1
#    else
#        define REAL_DIR_ENTRY(dp) ((dp)->d_ino != 0)
#    endif
#else  /* NODIR */
#    define generate_backup_filename(v, f) simple_backup_name((f))
#endif /* NODIR */

#ifndef BACKUP_SUFFIX_STR
#    define BACKUP_SUFFIX_STR "~"
#endif

#ifndef BACKUP_SUFFIX_CHAR
#    define BACKUP_SUFFIX_CHAR '~'
#endif

#ifndef BACKUP_SUFFIX_FORMAT
#    define BACKUP_SUFFIX_FORMAT "%s.~%0*d~"
#endif

/**
 * Default backup file suffix to use
 */
static char *simple_backup_suffix = BACKUP_SUFFIX_STR;

/**
 * What kinds of backup files to make -- see table `version_control_values' below.
 */
backup_mode_t version_control = unknown;
int            version_width   = 1;

/**
 * Construct a simple backup name for PATHNAME by appending
 * the value of `simple_backup_suffix'.
 */
static char *
simple_backup_name(char *pathname)
{
    char *backup_name;
    backup_name = xmalloc(strlen(pathname) + strlen(simple_backup_suffix) + 2);
    sprintf(backup_name, "%s%s", pathname, simple_backup_suffix);
    return backup_name;
}

#ifndef NODIR
/* If DIRENTRY is a numbered backup version of file BASE, return
 * that number.  BASE_LENGTH is the string length of BASE. */
static int
version_number(char *base, char *direntry, int base_length)
{
    int   version;
    char *p = NULL;
    version = 0;
    if (!strncmp(base, direntry, base_length) && ISDIGIT(direntry[base_length + 2]))
    {
        for (p = &direntry[base_length + 2]; ISDIGIT(*p); ++p)
        {
            version = version * 10 + *p - '0';
        }
        if (p[0] != BACKUP_SUFFIX_CHAR || p[1])
        {
            version = 0;
        }
    }
    return version;
}

/* Return the highest version of file FILENAME in directory
 * DIRNAME.  Return`s 0 if there are no numbered versions. */
static int
highest_version(char *filename, char *dirname)
{
    DIR           *dirp = opendir(dirname);
    struct dirent *dp   = NULL;
    int            highestVersion;
    if (!dirp)
    {
        return 0;
    }
    int          this_version;
    unsigned int file_name_length = strlen(filename);
    highestVersion                = 0;
    while ((dp = readdir(dirp)) != 0)
    {
        if (!REAL_DIR_ENTRY(dp) || NAMLEN(dp) <= file_name_length + 2)
        {
            continue;
        }
        this_version = version_number(filename, dp->d_name, file_name_length);
        if (this_version > highestVersion)
        {
            highestVersion = this_version;
        }
    }
    closedir(dirp);
    return highestVersion;
}

/**
 * Return the highest version number for file PATHNAME.  If there
 * are no backups, or only a simple backup, return 0.
 */
static int
max_version(char *pathname)
{
    char *p;
    char *filename;
    int   pathlen = strlen(pathname);
    int   version;
    p = pathname + pathlen - 1;
    while ((p > pathname) && (*p != '/'))
    {
        p--;
    }
    if (*p == '/')
    {
        int   dirlen = p - pathname;
        char *dirname;
        filename = p + 1;
        dirname  = xmalloc(dirlen + 1);
        strncpy(dirname, pathname, (dirlen));
        dirname[dirlen] = '\0';
        version         = highest_version(filename, dirname);
        xfree(dirname);
    }
    else
    {
        filename = pathname;
        version  = highest_version(filename, ".");
    }
    return version;
}

/* Generate a backup filename for PATHNAME, dependent on the value of 'VERSION_CONTROL'. */
static char *
generate_backup_filename(backup_mode_t versionControl, char *pathname)
{
    int   last_numbered_version;
    char *backup_name;
    if (versionControl == none)
    {
        backup_name = NULL;
        return backup_name;
    }
    if (versionControl == simple)
    {
        backup_name = simple_backup_name(pathname);
    }
    else
    {
        last_numbered_version = max_version(pathname);
        if ((versionControl == numbered_existing) && (last_numbered_version == 0))
        {
            backup_name = simple_backup_name(pathname);
        }
        else
        {
            last_numbered_version++;
            backup_name = xmalloc(strlen(pathname) + 16);
            if (backup_name)
            {
                sprintf(backup_name, BACKUP_SUFFIX_FORMAT, pathname, version_width, (int)last_numbered_version);
            }
        }
    }
    return backup_name;
}
#endif                               /* !NODIR */

static version_control_values_t values[] = {
    {             none,    "never"}, /* Don't make backups.            */
    {             none,     "none"}, /* Ditto                          */
    {           simple,   "simple"}, /* Only simple backups            */
    {numbered_existing, "existing"}, /* Numbered if they already exist */
    {numbered_existing,      "nil"}, /* Ditto                          */
    {         numbered, "numbered"}, /* Numbered backups               */
    {         numbered,        "t"}, /* Ditto                          */
    {          unknown,          0}  /* Initial, undefined value.      */
};

/**
 * Determine the value of `version_control' by looking in the environment
 * variable 'VERSION_CONTROL'.  Defaults to numbered_existing.
 */
backup_mode_t
version_control_value(void)
{
    char                      *version = getenv("VERSION_CONTROL");
    version_control_values_t *v;
    backup_mode_t             ret = unknown;
    if ((version == NULL) || (*version == 0))
    {
        ret = numbered_existing;
    }
    else
    {
        v = &values[0];
        while (v->name)
        {
            if (strcmp(version, v->name) == 0)
            {
                ret = v->value;
                break;
            }
            else
            {
                v++;
            }
        }
    }
    return ret;
}

/* Initialize information used in determining backup filenames. */
static void
set_version_width(void)
{
    char *v = getenv("VERSION_WIDTH");
    if (v && ISDIGIT(*v))
    {
        version_width = atoi(v);
    }
    if (version_width > 16)
    {
        version_width = 16;
    }
}

void
initialize_backups(void)
{
    char *v = getenv("SIMPLE_BACKUP_SUFFIX");
    if (v && *v)
    {
        simple_backup_suffix = v;
    }
#ifdef NODIR
    version_control = simple;
#else  /* !NODIR */
    version_control = version_control_value();
    if (version_control == unknown)
    {
        fprintf(stderr, "indent:  Strange version-control value\n");
        fprintf(stderr, "indent:  Using numbered-existing\n");
        version_control = numbered_existing;
    }
#endif /* !NODIR */
    set_version_width();
}

/**
 * Make a backup copy of FILE, taking into account version-control.
 * See the description at the beginning of the file for details.
 */
void
make_backup(file_buffer_t *file, const struct stat *file_stats)
{
    FILE        *bf;
    char        *backup_filename;
    unsigned int size;
    if (version_control != none)
    {
        backup_filename = generate_backup_filename(version_control, file->name);
        if (!backup_filename)
        {
            fprintf(stderr, "indent: Can't make backup filename of %s\n", file->name);
            exit(system_error);
        }
        bf = fopen(backup_filename, "w");
        if (!bf)
        {
            fatal("Can't open backup file %s", backup_filename);
        }
        size = fwrite(file->data, file->size, 1, bf);
        if (size != 1)
        {
            fatal("Can't write to backup file %s", backup_filename);
        }
        fclose(bf);
#ifdef PRESERVE_MTIME
        {
            struct utimbuf buf;
            buf.actime  = time(NULL);
            buf.modtime = file_stats->st_mtime;
            if (utime(backup_filename, &buf) != 0)
            {
                WARNING(_("Can't preserve modification time on backup file %s"), backup_filename, NULL);
            }
        }
#endif
        xfree(backup_filename);
    }
}
