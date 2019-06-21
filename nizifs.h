#ifndef NIZIFS_H
#define NIZIFS_H

#define NIZI_FS_TYPE 0x13090D15         /* Magic Number for our file system */
#define NIZI_FS_BLOCK_SIZE 512          /* in bytes */
#define NIZI_FS_ENTRY_SIZE 64           /* in bytes */
#define NIZI_FS_BLOCK_SIZE_BITS 9       /* log(SIMULA_FS_BLOCK_SIZE) w/ base 2 */
#define NIZI_FS_FILENAME_LEN 16         /* so max length is 15 */
#define NIZI_FS_DATA_BLOCK_CNT ((NIZI_FS_ENTRY_SIZE - (NIZI_FS_FILENAME_LEN + 3 * 4)) / 4)

#define NIZI_BACKING_FILE ".nizifs.img"

typedef unsigned int uint4_t;

typedef struct nizifs_super_block
{
    uint4_t type;                       /* Magic number to identify the file system */
    uint4_t block_size;                 /* Unit of allocation */
    uint4_t partition_size;             /* in blocks */
    uint4_t entry_size;                 /* in bytes */
    uint4_t entry_table_size;           /* in blocks */
    uint4_t entry_table_block_start;    /* in blocks */
    uint4_t entry_count;                /* Total entries in the file system */
    uint4_t data_block_start;           /* in blocks */
    uint4_t reserved[NIZI_FS_BLOCK_SIZE / 4 - 8];   /* Making it of NIZI_FS_BLOCK_SIZE */
} nizifs_super_block_t;

typedef struct nizifs_file_entry
{
    char name[NIZI_FS_FILENAME_LEN];
    uint4_t size;                       /* in bytes */
    uint4_t timestamp;                  /* Seconds since Epoch */
    uint4_t perms;                      /* Permissions for user */
    uint4_t blocks[NIZI_FS_DATA_BLOCK_CNT];
} nizifs_file_entry_t;

#endif
