#ifndef REAL_IO_H
#define REAL_IO_H

int read_sb_from_nizifs(nizifs_info_t *info, nizifs_super_block_t *sb);

int read_entry_from_nizifs(nizifs_info_t *info, int ino, nizifs_file_entry_t *fe);

int nizifs_update(nizifs_info_t *info, int vfs_ino, int *size, int *timestamp, int *perms);


int nizifs_lookup_file(nizifs_info_t *info, char *fn, nizifs_file_entry_t *fe);
int nizifs_create_file(nizifs_info_t *info, char *fn, int perms, nizifs_file_entry_t *fe);
int nizifs_remove_file(nizifs_info_t *info, char *fn);

#endif
