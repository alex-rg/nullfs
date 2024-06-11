/*
    FUSE: Filesystem in Userspace
    Copyright (C) 2001-2005  Miklos Szeredi <miklos@szeredi.hu>

    This program can be distributed under the terms of the GNU GPL.
    See the file COPYING.
*/

#define _FILE_OFFSET_BITS 64
#define FUSE_USE_VERSION 26
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <time.h>
#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>

#define MAX_MIRROR_LENGTH 1000
#define MAX_PATH_LENGTH 10000

#define MAX_DIRS 1024
#define MAX_FILES 1024
#define MAX_FILENAME_LENGTH 1024
const char* MOUNT = "/mnt";

char FILES[MAX_FILES][MAX_FILENAME_LENGTH];
char DIRS[MAX_DIRS][MAX_FILENAME_LENGTH];
unsigned int CUR_DIR_IDX = 0;
unsigned int CUR_FILE_IDX = 0;
time_t start_t = 0;

int tmpi = 0;

static int strendswith(const char *str, const char *sfx) {
    size_t sfx_len = strlen(sfx);
    size_t str_len = strlen(str);
    if (str_len < sfx_len) return 0;
    return (strncmp(str + (str_len - sfx_len), sfx, sfx_len) == 0);
};

static int nullfs_isdir(const char *path) {
    int res = 0;
    int i;

    for (i=0; i<MAX_DIRS; i++) {
        if (strcmp(path, DIRS[i]) == 0) {
            res = 1; 
            break;
        }
    }
    return res;
};

static int nullfs_isfile(const char *path) {
    int res = 0;
    int i;

    for (i=0; i<MAX_FILES; i++) {
        if (strcmp(path, FILES[i]) == 0) {
            res = 1; 
            break;
        }
    }
    return res;
};


static int nullfs_getattr(const char *path, struct stat *stbuf) {
    int i;
    if (nullfs_isdir(path) || strncmp(path, "/", MAX_FILENAME_LENGTH) == 0) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = start_t;
        stbuf->st_ctime = start_t;
    } else if (nullfs_isfile(path)) {
        stbuf->st_mode = S_IFREG | 0666;
        stbuf->st_nlink = 1;
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = start_t;
        stbuf->st_ctime = start_t;
    } else {
        return -ENOENT;
    }
    return 0;
};

static int nullfs_readdir(const char *path, void *buf, fuse_fill_dir_t
filler, off_t offset, struct fuse_file_info *fi) {
    (void) offset;
    (void) fi;

    if (! nullfs_isdir(path)) return -ENOENT;


    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    return 0;
};

static int nullfs_open(const char *path, struct fuse_file_info *fi) {
    (void) fi;

    //if (fi->flags & O_RDONLY) { 
    //  return -ENOENT;
    //} else {
    return 0;
    //}

};

static int nullfs_read(const char *path, char *buf, size_t size,
off_t offset, struct fuse_file_info *fi) {
    (void) buf;
    (void) size;
    (void) offset;
    (void) fi;

    /*if (nullfs_isdir(path)) return -ENOENT;*/

    return -1;
};

static int nullfs_write(const char *path, const char *buf, size_t size,
off_t offset, struct fuse_file_info *fi) {
    (void) buf;
    (void) offset;
    (void) fi;

    /*if (nullfs_isdir(path)) return -ENOENT;*/

    return -ENOENT;
    /*return (int) size;*/
};

static int nullfs_create(const char *path, mode_t m,
struct fuse_file_info *fi) {
    (void) path;
    (void) m;
    (void) fi;
    if (nullfs_isfile(path)) {
        return -EEXIST;
    } else {
       CUR_FILE_IDX = CUR_FILE_IDX % MAX_FILES;
       strncpy(FILES[CUR_FILE_IDX], path, MAX_FILENAME_LENGTH-1 );
       CUR_FILE_IDX += 1;
       return 0;
    }
};

static int nullfs_unlink(const char *path) {
    (void) path;

    return 0;
};

static int nullfs_rename(const char *src, const char *dst) {
    (void) src;
    (void) dst;

    return 0;
};

static int nullfs_truncate(const char *path, off_t o) {
    (void) path;
    (void) o;

    return 0;
};

static int nullfs_chmod(const char *path, mode_t m) {
    (void) path;
    (void) m;

    return 0;
};

static int nullfs_chown(const char *path, uid_t u, gid_t g) {
    (void) path;
    (void) u;
    (void) g;

    return 0;
};

static int nullfs_utimens(const char *path, const struct timespec ts[2]) {
    (void) path;
    (void) ts;

    return 0;
};

static int nullfs_statfs(const char *path, struct statvfs* vfs) {
    (void) path;
    vfs->f_bsize = 1024;
    vfs->f_frsize = 0;
    vfs->f_blocks = 1024*1024*1024;
    vfs->f_bfree = 1024*1024*1024;
    vfs->f_bavail = 1024*1024*1024;
    vfs->f_files = 0;
    vfs->f_ffree = 1024*1024*1024;
    vfs->f_favail = 1024*1024*1024;

    return 0;
};

static int nullfs_mkdir(const char *path, mode_t mode) {
    if (nullfs_isdir(path)) {
        return -EEXIST;
    } else {
        CUR_DIR_IDX = CUR_DIR_IDX % MAX_DIRS;
        strncpy(DIRS[CUR_DIR_IDX], path, MAX_FILENAME_LENGTH-1 );
        CUR_DIR_IDX += 1;
        return 0;
    }
};


static struct fuse_operations nullfs_oper = {
    .getattr    = nullfs_getattr,
    .readdir    = nullfs_readdir,
    .open       = nullfs_open,
    .read       = nullfs_read,
    .write      = nullfs_write,
    .create     = nullfs_create,
    .unlink     = nullfs_unlink,
    .rmdir      = nullfs_unlink,
    .truncate   = nullfs_truncate,
    .rename     = nullfs_rename,
    .chmod      = nullfs_chmod,
    .chown      = nullfs_chown,
    .utimens    = nullfs_utimens,
    .statfs     = nullfs_statfs,
    .mkdir      = nullfs_mkdir,
};

int main(int argc, char *argv[]) {
    start_t = time(NULL);
    return fuse_main(argc, argv, &nullfs_oper, NULL);
};

/* vi:set sw=4 et tw=72: */
