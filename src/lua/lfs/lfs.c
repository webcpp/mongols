/*
 ** LuaFileSystem
 ** Copyright Kepler Project 2003 - 2017 (http://keplerproject.github.io/luafilesystem)
 **
 ** File system manipulation library.
 ** This library offers these functions:
 **   lfs.attributes (filepath [, attributename | attributetable])
 **   lfs.chdir (path)
 **   lfs.currentdir ()
 **   lfs.dir (path)
 **   lfs.link (old, new[, symlink])
 **   lfs.lock (fh, mode)
 **   lfs.lock_dir (path)
 **   lfs.mkdir (path)
 **   lfs.rmdir (path)
 **   lfs.setmode (filepath, mode)
 **   lfs.symlinkattributes (filepath [, attributename])
 **   lfs.touch (filepath [, atime [, mtime]])
 **   lfs.unlock (fh)
 */


#define _FILE_OFFSET_BITS 64 /* Linux, Solaris and HP-UX */


#ifndef LFS_DO_NOT_USE_LARGE_FILE
#define _LARGEFILE64_SOURCE
#endif

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>



#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <sys/param.h> /* for MAXPATHLEN */
#define LFS_MAXPATHLEN MAXPATHLEN


#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "lfs.h"

#define LFS_VERSION "1.7.0"
#define LFS_LIBNAME "lfs"

#if LUA_VERSION_NUM >= 503 /* Lua 5.3 */

#ifndef luaL_optlong
#define luaL_optlong luaL_optinteger
#endif

#endif

#if LUA_VERSION_NUM >= 502
#define new_lib(L, l) (luaL_newlib(L, l))
#else
#define new_lib(L, l) (lua_newtable(L), luaL_register(L, NULL, l))
#endif

/* Define 'strerror' for systems that do not implement it */
#ifdef NO_STRERROR
#define strerror(_)     "System unable to describe the error"
#endif

#define DIR_METATABLE "directory metatable"

typedef struct dir_data {
    int closed;
    DIR *dir;
} dir_data;

#define LOCK_METATABLE "lock metatable"


#define _O_TEXT               0
#define _O_BINARY             0
#define lfs_setmode(file, m)   ((void)file, (void)m, 0)


#define lfs_mkdir(path) (mkdir((path), \
    S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH))

/*
 ** Utility functions
 */
static int pusherror(lua_State *L, const char *info) {
    lua_pushnil(L);
    if (info == NULL)
        lua_pushstring(L, strerror(errno));
    else
        lua_pushfstring(L, "%s: %s", info, strerror(errno));
    lua_pushinteger(L, errno);
    return 3;
}

static int pushresult(lua_State *L, int res, const char *info) {
    if (res == -1) {
        return pusherror(L, info);
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

/*
 ** This function changes the working (current) directory
 */
static int change_dir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    if (chdir(path)) {
        lua_pushnil(L);
        lua_pushfstring(L, "Unable to change working directory to '%s'\n%s\n",
                path, chdir_error);
        return 2;
    } else {
        lua_pushboolean(L, 1);
        return 1;
    }
}

/*
 ** This function returns the current directory
 ** If unable to get the current directory, it returns nil
 **  and a string describing the error
 */
static int get_dir(lua_State *L) {
#ifdef NO_GETCWD
    lua_pushnil(L);
    lua_pushstring(L, "Function 'getcwd' not provided by system");
    return 2;
#else
    char *path = NULL;
    /* Passing (NULL, 0) is not guaranteed to work. Use a temp buffer and size instead. */
    size_t size = LFS_MAXPATHLEN; /* initial buffer size */
    int result;
    while (1) {
        path = realloc(path, size);
        if (!path) /* failed to allocate */
            return pusherror(L, "get_dir realloc() failed");
        if (getcwd(path, size) != NULL) {
            /* success, push the path to the Lua stack */
            lua_pushstring(L, path);
            result = 1;
            break;
        }
        if (errno != ERANGE) { /* unexpected error */
            result = pusherror(L, "get_dir getcwd() failed");
            break;
        }
        /* ERANGE = insufficient buffer capacity, double size and retry */
        size *= 2;
    }
    free(path);
    return result;
#endif
}

/*
 ** Check if the given element on the stack is a file and returns it.
 */
static FILE *check_file(lua_State *L, int idx, const char *funcname) {
#if LUA_VERSION_NUM == 501
    FILE **fh = (FILE **) luaL_checkudata(L, idx, "FILE*");
    if (*fh == NULL) {
        luaL_error(L, "%s: closed file", funcname);
        return 0;
    } else
        return *fh;
#elif LUA_VERSION_NUM >= 502 && LUA_VERSION_NUM <= 503
    luaL_Stream *fh = (luaL_Stream *) luaL_checkudata(L, idx, "FILE*");
    if (fh->closef == 0 || fh->f == NULL) {
        luaL_error(L, "%s: closed file", funcname);
        return 0;
    } else
        return fh->f;
#else
#error unsupported Lua version
#endif
}

/*
 **
 */
static int _file_lock(lua_State *L, FILE *fh, const char *mode, const long start, long len, const char *funcname) {
    int code;
    struct flock f;
    switch (*mode) {
        case 'w': f.l_type = F_WRLCK;
            break;
        case 'r': f.l_type = F_RDLCK;
            break;
        case 'u': f.l_type = F_UNLCK;
            break;
        default: return luaL_error(L, "%s: invalid mode", funcname);
    }
    f.l_whence = SEEK_SET;
    f.l_start = (off_t) start;
    f.l_len = (off_t) len;
    code = fcntl(fileno(fh), F_SETLK, &f);

    return (code != -1);
}

typedef struct lfs_Lock {
    char *ln;
} lfs_Lock;

static int lfs_lock_dir(lua_State *L) {
    lfs_Lock *lock;
    size_t pathl;
    char *ln;
    const char *lockfile = "/lockfile.lfs";
    const char *path = luaL_checklstring(L, 1, &pathl);
    lock = (lfs_Lock*) lua_newuserdata(L, sizeof (lfs_Lock));
    ln = (char*) malloc(pathl + strlen(lockfile) + 1);
    if (!ln) {
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    strcpy(ln, path);
    strcat(ln, lockfile);
    if (symlink("lock", ln) == -1) {
        free(ln);
        lua_pushnil(L);
        lua_pushstring(L, strerror(errno));
        return 2;
    }
    lock->ln = ln;
    luaL_getmetatable(L, LOCK_METATABLE);
    lua_setmetatable(L, -2);
    return 1;
}

static int lfs_unlock_dir(lua_State *L) {
    lfs_Lock *lock = (lfs_Lock *) luaL_checkudata(L, 1, LOCK_METATABLE);
    if (lock->ln) {
        unlink(lock->ln);
        free(lock->ln);
        lock->ln = NULL;
    }
    return 0;
}

static int lfs_g_setmode(lua_State *L, FILE *f, int arg) {
    static const int mode[] = {_O_BINARY, _O_TEXT};
    static const char *const modenames[] = {"binary", "text", NULL};
    int op = luaL_checkoption(L, arg, NULL, modenames);
    int res = lfs_setmode(f, mode[op]);
    if (res != -1) {
        int i;
        lua_pushboolean(L, 1);
        for (i = 0; modenames[i] != NULL; i++) {
            if (mode[i] == res) {
                lua_pushstring(L, modenames[i]);
                return 2;
            }
        }
        lua_pushnil(L);
        return 2;
    } else {
        return pusherror(L, NULL);
    }
}

static int lfs_f_setmode(lua_State *L) {
    return lfs_g_setmode(L, check_file(L, 1, "setmode"), 2);
}

/*
 ** Locks a file.
 ** @param #1 File handle.
 ** @param #2 String with lock mode ('w'rite, 'r'ead).
 ** @param #3 Number with start position (optional).
 ** @param #4 Number with length (optional).
 */
static int file_lock(lua_State *L) {
    FILE *fh = check_file(L, 1, "lock");
    const char *mode = luaL_checkstring(L, 2);
    const long start = (long) luaL_optinteger(L, 3, 0);
    long len = (long) luaL_optinteger(L, 4, 0);
    if (_file_lock(L, fh, mode, start, len, "lock")) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "%s", strerror(errno));
        return 2;
    }
}

/*
 ** Unlocks a file.
 ** @param #1 File handle.
 ** @param #2 Number with start position (optional).
 ** @param #3 Number with length (optional).
 */
static int file_unlock(lua_State *L) {
    FILE *fh = check_file(L, 1, "unlock");
    const long start = (long) luaL_optinteger(L, 2, 0);
    long len = (long) luaL_optinteger(L, 3, 0);
    if (_file_lock(L, fh, "u", start, len, "unlock")) {
        lua_pushboolean(L, 1);
        return 1;
    } else {
        lua_pushnil(L);
        lua_pushfstring(L, "%s", strerror(errno));
        return 2;
    }
}

/*
 ** Creates a link.
 ** @param #1 Object to link to.
 ** @param #2 Name of link.
 ** @param #3 True if link is symbolic (optional).
 */
static int make_link(lua_State *L) {
    const char *oldpath = luaL_checkstring(L, 1);
    const char *newpath = luaL_checkstring(L, 2);
    //  int res = (lua_toboolean(L,3) ? symlink : link)(oldpath, newpath);
    int res;
    if (lua_toboolean(L, 3)) {
        res = symlink(oldpath, newpath);
    } else {
        res = link(oldpath, newpath);
    }
    if (res == -1) {
        return pusherror(L, NULL);
    } else {
        lua_pushinteger(L, 0);
        return 1;
    }
}

/*
 ** Creates a directory.
 ** @param #1 Directory path.
 */
static int make_dir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    return pushresult(L, lfs_mkdir(path), NULL);
}

/*
 ** Removes a directory.
 ** @param #1 Directory path.
 */
static int remove_dir(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    return pushresult(L, rmdir(path), NULL);
}

/*
 ** Directory iterator
 */
static int dir_iter(lua_State *L) {

    struct dirent *entry;

    dir_data *d = (dir_data *) luaL_checkudata(L, 1, DIR_METATABLE);
    luaL_argcheck(L, d->closed == 0, 1, "closed directory");
    if ((entry = readdir(d->dir)) != NULL) {
        lua_pushstring(L, entry->d_name);
        return 1;
    } else {
        /* no more entries => close directory */
        closedir(d->dir);
        d->closed = 1;
        return 0;
    }

}

/*
 ** Closes directory iterators
 */
static int dir_close(lua_State *L) {
    dir_data *d = (dir_data *) lua_touserdata(L, 1);
    if (!d->closed && d->dir) {
        closedir(d->dir);
    }
    d->closed = 1;
    return 0;
}

/*
 ** Factory of directory iterators
 */
static int dir_iter_factory(lua_State *L) {
    const char *path = luaL_checkstring(L, 1);
    dir_data *d;
    lua_pushcfunction(L, dir_iter);
    d = (dir_data *) lua_newuserdata(L, sizeof (dir_data));
    luaL_getmetatable(L, DIR_METATABLE);
    lua_setmetatable(L, -2);
    d->closed = 0;

    d->dir = opendir(path);
    if (d->dir == NULL)
        luaL_error(L, "cannot open %s: %s", path, strerror(errno));

    return 2;
}

/*
 ** Creates directory metatable.
 */
static int dir_create_meta(lua_State *L) {
    luaL_newmetatable(L, DIR_METATABLE);

    /* Method table */
    lua_newtable(L);
    lua_pushcfunction(L, dir_iter);
    lua_setfield(L, -2, "next");
    lua_pushcfunction(L, dir_close);
    lua_setfield(L, -2, "close");

    /* Metamethods */
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, dir_close);
    lua_setfield(L, -2, "__gc");
    return 1;
}

/*
 ** Creates lock metatable.
 */
static int lock_create_meta(lua_State *L) {
    luaL_newmetatable(L, LOCK_METATABLE);

    /* Method table */
    lua_newtable(L);
    lua_pushcfunction(L, lfs_unlock_dir);
    lua_setfield(L, -2, "free");

    /* Metamethods */
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, lfs_unlock_dir);
    lua_setfield(L, -2, "__gc");
    return 1;
}

/*
 ** Convert the inode protection mode to a string.
 */


static const char *mode2string(mode_t mode) {
    if (S_ISREG(mode))
        return "file";
    else if (S_ISDIR(mode))
        return "directory";
    else if (S_ISLNK(mode))
        return "link";
    else if (S_ISSOCK(mode))
        return "socket";
    else if (S_ISFIFO(mode))
        return "named pipe";
    else if (S_ISCHR(mode))
        return "char device";
    else if (S_ISBLK(mode))
        return "block device";
    else
        return "other";
}

/*
 ** Set access time and modification values for a file.
 ** @param #1 File path.
 ** @param #2 Access time in seconds, current time is used if missing.
 ** @param #3 Modification time in seconds, access time is used if missing.
 */
static int file_utime(lua_State *L) {
    const char *file = luaL_checkstring(L, 1);
    struct utimbuf utb, *buf;

    if (lua_gettop(L) == 1) /* set to current date/time */
        buf = NULL;
    else {
        utb.actime = (time_t) luaL_optnumber(L, 2, 0);
        utb.modtime = (time_t) luaL_optinteger(L, 3, utb.actime);
        buf = &utb;
    }

    return pushresult(L, utime(file, buf), NULL);
}

/* inode protection mode */
static void push_st_mode(lua_State *L, struct stat *info) {
    lua_pushstring(L, mode2string(info->st_mode));
}

/* device inode resides on */
static void push_st_dev(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_dev);
}

/* inode's number */
static void push_st_ino(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_ino);
}

/* number of hard links to the file */
static void push_st_nlink(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_nlink);
}

/* user-id of owner */
static void push_st_uid(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_uid);
}

/* group-id of owner */
static void push_st_gid(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_gid);
}

/* device type, for special file inode */
static void push_st_rdev(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_rdev);
}

/* time of last access */
static void push_st_atime(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_atime);
}

/* time of last data modification */
static void push_st_mtime(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_mtime);
}

/* time of last file status change */
static void push_st_ctime(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_ctime);
}

/* file size, in bytes */
static void push_st_size(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_size);
}

/* blocks allocated for file */
static void push_st_blocks(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_blocks);
}

/* optimal file system I/O blocksize */
static void push_st_blksize(lua_State *L, struct stat *info) {
    lua_pushinteger(L, (lua_Integer) info->st_blksize);
}

/*
 ** Convert the inode protection mode to a permission list.
 */


static const char *perm2string(mode_t mode) {
    static char perms[10] = "---------";
    int i;
    for (i = 0; i < 9; i++) perms[i] = '-';
    if (mode & S_IRUSR) perms[0] = 'r';
    if (mode & S_IWUSR) perms[1] = 'w';
    if (mode & S_IXUSR) perms[2] = 'x';
    if (mode & S_IRGRP) perms[3] = 'r';
    if (mode & S_IWGRP) perms[4] = 'w';
    if (mode & S_IXGRP) perms[5] = 'x';
    if (mode & S_IROTH) perms[6] = 'r';
    if (mode & S_IWOTH) perms[7] = 'w';
    if (mode & S_IXOTH) perms[8] = 'x';
    return perms;
}

/* permssions string */
static void push_st_perm(lua_State *L, struct stat *info) {
    lua_pushstring(L, perm2string(info->st_mode));
}

typedef void (*_push_function) (lua_State *L, struct stat *info);

struct _stat_members {
    const char *name;
    _push_function push;
};

struct _stat_members members[] = {
    { "mode", push_st_mode},
    { "dev", push_st_dev},
    { "ino", push_st_ino},
    { "nlink", push_st_nlink},
    { "uid", push_st_uid},
    { "gid", push_st_gid},
    { "rdev", push_st_rdev},
    { "access", push_st_atime},
    { "modification", push_st_mtime},
    { "change", push_st_ctime},
    { "size", push_st_size},
    { "permissions", push_st_perm},
    { "blocks", push_st_blocks},
    { "blksize", push_st_blksize},
    { NULL, NULL}
};

/*
 ** Get file or symbolic link information
 */
static int _file_info_(lua_State *L) {
    struct stat info;
    const char *file = luaL_checkstring(L, 1);
    int i;

    if (stat(file, &info)) {
        lua_pushnil(L);
        lua_pushfstring(L, "cannot obtain information from file '%s': %s", file, strerror(errno));
        lua_pushinteger(L, errno);
        return 3;
    }
    if (lua_isstring(L, 2)) {
        const char *member = lua_tostring(L, 2);
        for (i = 0; members[i].name; i++) {
            if (strcmp(members[i].name, member) == 0) {
                /* push member value and return */
                members[i].push(L, &info);
                return 1;
            }
        }
        /* member not found */
        return luaL_error(L, "invalid attribute name '%s'", member);
    }
    /* creates a table if none is given, removes extra arguments */
    lua_settop(L, 2);
    if (!lua_istable(L, 2)) {
        lua_newtable(L);
    }
    /* stores all members in table on top of the stack */
    for (i = 0; members[i].name; i++) {
        lua_pushstring(L, members[i].name);
        members[i].push(L, &info);
        lua_rawset(L, -3);
    }
    return 1;
}

/*
 ** Get file information using stat.
 */
static int file_info(lua_State *L) {
    return _file_info_(L);
}

/*
 ** Push the symlink target to the top of the stack.
 ** Assumes the file name is at position 1 of the stack.
 ** Returns 1 if successful (with the target on top of the stack),
 ** 0 on failure (with stack unchanged, and errno set).
 */
static int push_link_target(lua_State *L) {

    const char *file = luaL_checkstring(L, 1);
    char *target = NULL;
    int tsize, size = 256; /* size = initial buffer capacity */
    while (1) {
        target = realloc(target, size);
        if (!target) /* failed to allocate */
            return 0;
        tsize = readlink(file, target, size);
        if (tsize < 0) { /* a readlink() error occurred */
            free(target);
            return 0;
        }
        if (tsize < size)
            break;
        /* possibly truncated readlink() result, double size and retry */
        size *= 2;
    }
    target[tsize] = '\0';
    lua_pushlstring(L, target, tsize);
    free(target);
    return 1;

}

/*
 ** Get symbolic link information using lstat.
 */
static int link_info(lua_State *L) {
    int ret;
    if (lua_isstring(L, 2) && (strcmp(lua_tostring(L, 2), "target") == 0)) {
        int ok = push_link_target(L);
        return ok ? 1 : pusherror(L, "could not obtain link target");
    }
    ret = _file_info_(L);
    if (ret == 1 && lua_type(L, -1) == LUA_TTABLE) {
        int ok = push_link_target(L);
        if (ok) {
            lua_setfield(L, -2, "target");
        }
    }
    return ret;
}

/*
 ** Assumes the table is on top of the stack.
 */
static void set_info(lua_State *L) {
    lua_pushliteral(L, "Copyright (C) 2003-2017 Kepler Project");
    lua_setfield(L, -2, "_COPYRIGHT");
    lua_pushliteral(L, "LuaFileSystem is a Lua library developed to complement the set of functions related to file systems offered by the standard Lua distribution");
    lua_setfield(L, -2, "_DESCRIPTION");
    lua_pushliteral(L, "LuaFileSystem " LFS_VERSION);
    lua_setfield(L, -2, "_VERSION");
}


static const struct luaL_Reg fslib[] = {
    {"attributes", file_info},
    {"chdir", change_dir},
    {"currentdir", get_dir},
    {"dir", dir_iter_factory},
    {"link", make_link},
    {"lock", file_lock},
    {"mkdir", make_dir},
    {"rmdir", remove_dir},
    {"symlinkattributes", link_info},
    {"setmode", lfs_f_setmode},
    {"touch", file_utime},
    {"unlock", file_unlock},
    {"lock_dir", lfs_lock_dir},
    {NULL, NULL},
};

LFS_EXPORT int luaopen_lfs(lua_State *L) {
    dir_create_meta(L);
    lock_create_meta(L);
    new_lib(L, fslib);
    lua_pushvalue(L, -1);
    lua_setglobal(L, LFS_LIBNAME);
    set_info(L);
    return 1;
}
