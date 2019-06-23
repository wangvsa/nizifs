#ifndef REAL_IO_H
#define REAL_IO_H

int read_sb_from_nizifs(nizifs_info_t *info, nizifs_super_block_t *sb);

int read_entry_from_nizifs(nizifs_info_t *info, int ino, nizifs_file_entry_t *fe);

int nizifs_update(nizifs_info_t *info, int vfs_ino, int *size, int *timestamp, int *perms);

#endif
