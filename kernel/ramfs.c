/* SYNAPSE SO - RAM Filesystem Implementation */
/* Licensed under GPLv3 */

#include <kernel/ramfs.h>
#include <kernel/heap.h>
#include <kernel/string.h>
#include <kernel/vga.h>
#include <kernel/vfs.h>

/* Maximum files in ramfs */
#define RAMFS_MAX_FILES 64
#define RAMFS_MAX_NAME 64
#define RAMFS_MAX_SIZE 4096

/* RAM file structure */
typedef struct {
    char name[RAMFS_MAX_NAME];
    uint8_t* data;
    uint32_t size;
    uint32_t used;
    int in_use;
} ramfs_file_t;

/* RAM filesystem structure */
static ramfs_file_t ramfs_files[RAMFS_MAX_FILES];
static filesystem_t ramfs_fs;

/* Find a file by name */
static ramfs_file_t* ramfs_find_file(const char* name) {
    if (name == 0) {
        return 0;
    }

    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (ramfs_files[i].in_use && strcmp(ramfs_files[i].name, name) == 0) {
            return &ramfs_files[i];
        }
    }

    return 0;
}

/* Allocate a file slot */
static ramfs_file_t* ramfs_alloc_file(void) {
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        if (!ramfs_files[i].in_use) {
            return &ramfs_files[i];
        }
    }
    return 0;
}

/* RAMFS open operation */
static uint32_t ramfs_open(const char* path, int flags) {
    (void)flags;  /* Not used yet */

    /* Check if file exists */
    ramfs_file_t* file = ramfs_find_file(path);
    if (file != 0) {
        vga_print("[+] ramfs: Found file: ");
        vga_print(path);
        vga_print("\n");
        return (uint32_t)(file - ramfs_files) + 1;  /* Inode = index + 1 */
    }

    /* File doesn't exist, try to create it */
    file = ramfs_alloc_file();
    if (file == 0) {
        vga_print("[-] ramfs: No free file slots\n");
        return 0;
    }

    /* Initialize file */
    strncpy(file->name, path, RAMFS_MAX_NAME - 1);
    file->name[RAMFS_MAX_NAME - 1] = '\0';
    file->data = (uint8_t*)kmalloc(RAMFS_MAX_SIZE);
    if (file->data == 0) {
        vga_print("[-] ramfs: Failed to allocate file data\n");
        return 0;
    }
    file->size = RAMFS_MAX_SIZE;
    file->used = 0;
    file->in_use = 1;

    vga_print("[+] ramfs: Created file: ");
    vga_print(path);
    vga_print("\n");

    return (uint32_t)(file - ramfs_files) + 1;
}

/* RAMFS close operation */
static int ramfs_close(uint32_t inode) {
    if (inode == 0 || inode > RAMFS_MAX_FILES) {
        return -1;
    }

    ramfs_file_t* file = &ramfs_files[inode - 1];
    if (!file->in_use) {
        return -1;
    }

    vga_print("[+] ramfs: Closed file: ");
    vga_print(file->name);
    vga_print("\n");

    /* Don't free the file - keep it in RAM */
    return 0;
}

/* RAMFS read operation */
static int ramfs_read(uint32_t inode, void* buffer, uint32_t count, uint32_t offset) {
    if (inode == 0 || inode > RAMFS_MAX_FILES) {
        return -1;
    }

    ramfs_file_t* file = &ramfs_files[inode - 1];
    if (!file->in_use) {
        return -1;
    }

    /* Check offset */
    if (offset >= file->used) {
        return 0;  /* EOF */
    }

    /* Calculate bytes to read */
    uint32_t bytes_to_read = count;
    if (offset + bytes_to_read > file->used) {
        bytes_to_read = file->used - offset;
    }

    /* Copy data */
    memcpy(buffer, file->data + offset, bytes_to_read);

    vga_print("[+] ramfs: Read ");
    vga_print_dec(bytes_to_read);
    vga_print(" bytes from ");
    vga_print(file->name);
    vga_print("\n");

    return (int)bytes_to_read;
}

/* RAMFS write operation */
static int ramfs_write(uint32_t inode, const void* buffer, uint32_t count, uint32_t offset) {
    if (inode == 0 || inode > RAMFS_MAX_FILES) {
        return -1;
    }

    ramfs_file_t* file = &ramfs_files[inode - 1];
    if (!file->in_use) {
        return -1;
    }

    /* Check if write fits */
    if (offset + count > file->size) {
        vga_print("[-] ramfs: Write exceeds file size\n");
        return -1;
    }

    /* Copy data */
    memcpy(file->data + offset, buffer, count);

    /* Update used size */
    if (offset + count > file->used) {
        file->used = offset + count;
    }

    vga_print("[+] ramfs: Wrote ");
    vga_print_dec(count);
    vga_print(" bytes to ");
    vga_print(file->name);
    vga_print("\n");

    return (int)count;
}

/* Initialize RAM filesystem */
int ramfs_init(void) {
    vga_print("[+] Initializing RAM filesystem...\n");

    /* Clear all file slots */
    for (int i = 0; i < RAMFS_MAX_FILES; i++) {
        ramfs_files[i].data = 0;
        ramfs_files[i].size = 0;
        ramfs_files[i].used = 0;
        ramfs_files[i].in_use = 0;
    }

    /* Set up filesystem structure */
    strcpy(ramfs_fs.name, "ramfs");
    ramfs_fs.next = 0;
    ramfs_fs.open = ramfs_open;
    ramfs_fs.close = ramfs_close;
    ramfs_fs.read = ramfs_read;
    ramfs_fs.write = ramfs_write;

    /* Register with VFS */
    vfs_register_fs(&ramfs_fs);

    vga_print("    RAM filesystem initialized\n");
    return 0;
}

/* Create a file with initial content */
int ramfs_create_file(const char* name, const char* content) {
    uint32_t inode = ramfs_open(name, 0);
    if (inode == 0) {
        return -1;
    }

    if (content != 0) {
        uint32_t len = strlen(content);
        ramfs_write(inode, content, len, 0);
    }

    return 0;
}
