/* SYNAPSE SO - RAM Filesystem */
/* Licensed under GPLv3 */

#ifndef KERNEL_RAMFS_H
#define KERNEL_RAMFS_H

/* Initialize RAM filesystem */
int ramfs_init(void);

/* Create a file with initial content */
int ramfs_create_file(const char* name, const char* content);

#endif /* KERNEL_RAMFS_H */
