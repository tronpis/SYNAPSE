/* SYNAPSE SO - Virtual File System (VFS) */
/* Licensed under GPLv3 */

#include <kernel/vfs.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/vga.h>

/* Filesystem list */
static filesystem_t* fs_list = 0;

/* File descriptor table (per-process would be better) */
static file_t fd_table[MAX_OPEN_FILES];

/* Initialize VFS */
void vfs_init(void) {
    vga_print("[+] Initializing VFS...\n");

    /* Clear file descriptor table */
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        fd_table[i].inode = 0;
        fd_table[i].offset = 0;
        fd_table[i].fs = 0;
    }

    vga_print("    VFS initialized\n");
}

/* Register a filesystem */
void vfs_register_fs(filesystem_t* fs) {
    if (fs == 0) {
        return;
    }

    vga_print("[+] Registering filesystem: ");
    vga_print(fs->name);
    vga_print("\n");

    /* Add to filesystem list */
    if (fs_list == 0) {
        fs_list = fs;
        fs->next = 0;
    } else {
        filesystem_t* current = fs_list;
        while (current->next != 0) {
            current = current->next;
        }
        current->next = fs;
        fs->next = 0;
    }
}

/* Open a file */
int vfs_open(const char* path, int flags, int mode) {
    (void)mode;  /* Not used yet */

    if (path == 0) {
        return -1;
    }

    vga_print("[+] vfs_open: ");
    vga_print(path);
    vga_print("\n");

    /* Find a free file descriptor */
    int fd = -1;
    for (int i = 0; i < MAX_OPEN_FILES; i++) {
        if (fd_table[i].inode == 0) {
            fd = i;
            break;
        }
    }

    if (fd < 0) {
        vga_print("[-] vfs_open: No free file descriptors\n");
        return -1;
    }

    /* Try each filesystem */
    filesystem_t* fs = fs_list;
    while (fs != 0) {
        if (fs->open != 0) {
            uint32_t inode = fs->open(path, flags);
            if (inode != 0) {
                fd_table[fd].inode = inode;
                fd_table[fd].offset = 0;
                fd_table[fd].fs = fs;
                vga_print("[+] vfs_open: Opened with fd=");
                vga_print_dec(fd);
                vga_print("\n");
                return fd;
            }
        }
        fs = fs->next;
    }

    vga_print("[-] vfs_open: File not found\n");
    return -1;
}

/* Close a file */
int vfs_close(int fd) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }

    file_t* file = &fd_table[fd];
    if (file->inode == 0) {
        return -1;
    }

    vga_print("[+] vfs_close: fd=");
    vga_print_dec(fd);
    vga_print("\n");

    /* Call filesystem close */
    if (file->fs != 0 && file->fs->close != 0) {
        file->fs->close(file->inode);
    }

    /* Clear file descriptor */
    file->inode = 0;
    file->offset = 0;
    file->fs = 0;

    return 0;
}

/* Read from a file */
int vfs_read(int fd, void* buffer, uint32_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }

    file_t* file = &fd_table[fd];
    if (file->inode == 0 || file->fs == 0) {
        return -1;
    }

    /* Call filesystem read */
    if (file->fs->read != 0) {
        int bytes = file->fs->read(file->inode, buffer, count, file->offset);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }

    return -1;
}

/* Write to a file */
int vfs_write(int fd, const void* buffer, uint32_t count) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }

    file_t* file = &fd_table[fd];
    if (file->inode == 0 || file->fs == 0) {
        return -1;
    }

    /* Call filesystem write */
    if (file->fs->write != 0) {
        int bytes = file->fs->write(file->inode, buffer, count, file->offset);
        if (bytes > 0) {
            file->offset += bytes;
        }
        return bytes;
    }

    return -1;
}

/* Seek in a file */
int vfs_lseek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_OPEN_FILES) {
        return -1;
    }

    file_t* file = &fd_table[fd];
    if (file->inode == 0) {
        return -1;
    }

    uint32_t new_offset = file->offset;

    switch (whence) {
        case SEEK_SET:
            new_offset = offset;
            break;
        case SEEK_CUR:
            new_offset += offset;
            break;
        case SEEK_END:
            /* Need to get file size from filesystem */
            /* For now, just use current offset */
            break;
        default:
            return -1;
    }

    file->offset = new_offset;
    return (int)new_offset;
}
