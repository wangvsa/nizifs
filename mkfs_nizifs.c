#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "nizifs_ds.h"

#define NIZIFS_ENTRY_RATIO 0.10 /* 10% of all blocks */
#define NIZIFS_ENTRY_TABLE_BLOCK_START 1

nizifs_super_block_t sb =
{
    .type = NIZI_FS_TYPE,
    .block_size = NIZI_FS_BLOCK_SIZE,
    .entry_size = NIZI_FS_ENTRY_SIZE,
    .entry_table_block_start = NIZIFS_ENTRY_TABLE_BLOCK_START
};
nizifs_file_entry_t fe; /* All 0's */

void write_super_block(int nizifs_handle, nizifs_super_block_t *sb)
{
    write(nizifs_handle, sb, sizeof(nizifs_super_block_t));
}

void clear_file_entries(int nizifs_handle, nizifs_super_block_t *sb)
{
    for (int i = 0; i < sb->entry_count; i++)
        write(nizifs_handle, &fe, sizeof(fe));
}

void mark_data_blocks(int nizifs_handle, nizifs_super_block_t *sb)
{
    char c = 0;
    lseek(nizifs_handle, sb->partition_size * sb->block_size - 1, SEEK_SET);
    write(nizifs_handle, &c, 1); /* To make the file size to partition size */
}

int main(int argc, char *argv[])
{
    int nizifs_handle;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <partition size in 512-byte blocks>\n", argv[0]);
        return 1;
    }
    sb.partition_size = atoi(argv[1]);
    sb.entry_table_size = sb.partition_size * NIZIFS_ENTRY_RATIO;
    sb.entry_count = sb.entry_table_size * sb.block_size / sb.entry_size;
    printf("%d %d\n", sb.entry_table_size, sb.entry_count);
    sb.data_block_start = NIZIFS_ENTRY_TABLE_BLOCK_START + sb.entry_table_size;

    nizifs_handle = creat(NIZI_BACKING_FILE, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (nizifs_handle == -1)
    {
        perror("No permissions to format");
        return 2;
    }

    write_super_block(nizifs_handle, &sb);
    clear_file_entries(nizifs_handle, &sb);
    mark_data_blocks(nizifs_handle, &sb);
    close(nizifs_handle);

    return 0;
}
