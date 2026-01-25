/* SYNAPSE SO - Virtual File System (VFS) */
/* Licensed under GPLv3 */

#ifndef KERNEL_VFS_H
#define KERNEL_VFS_H

#include <stdint.h>

/* Maximum open files */
#define MAX_OPEN_FILES 256

/* Seek constants */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

/* File descriptor */
typedef struct {
    uint32_t inode;
    uint32_t offset;
    struct filesystem* fs;
} file_t;

/* Filesystem structure */
typedef struct filesystem {
    char name[32];
    struct filesystem* next;

    /* Filesystem operations */
    uint32_t (*open)(const char* path, int flags);
    int (*close)(uint32_t inode);
    int (*read)(uint32_t inode, void* buffer, uint32_t count, uint32_t offset);
    int (*write)(uint32_t inode, const void* buffer, uint32_t count, uint32_t offset);
} filesystem_t;

/* VFS initialization */
void vfs_init(void);

/* Register a filesystem */
void vfs_register_fs(filesystem_t* fs);

/* VFS operations */
int vfs_open(const char* path, int flags, int mode);
int vfs_close(int fd);
int vfs_read(int fd, void* buffer, uint32_t count);
int vfs_write(int fd, const void* buffer, uint32_t count);
int vfs_lseek(int fd, int offset, int whence);

#endif /* KERNEL_VFS_H */
