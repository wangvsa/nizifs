#ifndef NIZIFS_H
#define NIZIFS_H

#ifdef __KERNEL__
#include <linux/fs.h>
#include <linux/spinlock.h>
#endif


#define NIZI_FS_TYPE 0x13090D15         /* Magic Number for our file system */
#define NIZI_FS_BLOCK_SIZE 512          /* in bytes */
#define NIZI_FS_ENTRY_SIZE 64           /* in bytes */
#define NIZI_FS_BLOCK_SIZE_BITS 9       /* log(SIMULA_FS_BLOCK_SIZE) w/ base 2 */
#define NIZI_FS_FILENAME_LEN 16         /* so max length is 15 */
#define NIZI_FS_DATA_BLOCK_CNT ((NIZI_FS_ENTRY_SIZE - (NIZI_FS_FILENAME_LEN + 3 * 4)) / 4)

#define NIZI_BACKING_FILE ".nizifs.img"

typedef unsigned char byte1_t;
typedef unsigned int byte4_t;
typedef unsigned long long byte8_t;

typedef struct nizifs_super_block
{
    byte4_t type;                       /* Magic number to identify the file system */
    byte4_t block_size;                 /* Unit of allocation */
    byte4_t partition_size;             /* in blocks */
    byte4_t entry_size;                 /* in bytes */
    byte4_t entry_table_size;           /* in blocks */
    byte4_t entry_table_block_start;    /* in blocks */
    byte4_t entry_count;                /* Total entries in the file system */
    byte4_t data_block_start;           /* in blocks */
    byte4_t reserved[NIZI_FS_BLOCK_SIZE / 4 - 8];   /* Making it of NIZI_FS_BLOCK_SIZE */
} nizifs_super_block_t;

typedef struct nizifs_file_entry
{
    char name[NIZI_FS_FILENAME_LEN];
    byte4_t size;                       /* in bytes */
    byte4_t timestamp;                  /* Seconds since Epoch */
    byte4_t perms;                      /* Permissions for user */
    byte4_t blocks[NIZI_FS_DATA_BLOCK_CNT];
} nizifs_file_entry_t;

#ifdef __KERNEL__
typedef struct nizifs_info {
    struct super_block *vfs_sb;         // VFS' super block
    nizifs_super_block_t sb;            // our super block
    byte1_t *used_blocks;               // bitmap as used blocks tracker
    spinlock_t lock;                   // protect used_blocks
} nizifs_info_t;
#endif

/*
 * Inode number 0 is not treated as a valid inode number at many places,
 * including applications like ls. So, the inode numbering should start
 * at not less than 1. For example, making root inode's number 0, leads
 * the lookuped entries . & .. to be not shown in ls
 */
#define ROOT_INODE_NUM (1)
#define N2V_INODE_NUM(i) (ROOT_INODE_NUM + 1 + (i))     // NIZIFS to VFS
#define V2N_INODE_NUM(i) ((i) - (ROOT_INODE_NUM + 1))   // NIZIFS to SFSk
#define INV_INODE (-1)
#define INV_BLOCK (-1)





/*
 * Data declarations in VFS
 * Required to implement a filesystem
 */
/* super.c */
extern struct file_system_type nizifs;
extern const struct super_operations nizifs_sops;

/* file.c */
extern const struct inode_operations nizifs_iops;
extern const struct file_operations nizifs_fops;
extern const struct file_operations nizifs_dops;

/* inode.c */
//extern void ext2_set_file_ops(struct inode *inode);
extern const struct address_space_operations nizifs_aops;
extern const struct address_space_operations nizifs_nobh_aops;
//extern const struct iomap_ops ext2_iomap_ops;


#endif

