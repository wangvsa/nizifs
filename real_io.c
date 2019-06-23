#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/buffer_head.h>

#include "nizifs.h"
#include "real_io.h"

static int read_from_nizifs(nizifs_info_t *info, byte4_t block, byte4_t offset, void *buf, byte4_t len) {
    byte4_t block_size = info->sb.block_size;
    byte4_t bd_block_size = info->vfs_sb->s_bdev->bd_block_size;

    struct buffer_head *bh;

    // Translate the nizifs block position to underlying block device block position, for sb_bread()
    byte4_t abs = block * block_size + offset;
    block = abs / bd_block_size;
    offset = abs % bd_block_size;
    if (offset + len > bd_block_size) // Should never happen
        return -EINVAL;
    if (!(bh = sb_bread(info->vfs_sb, block)))
        return -EIO;

    memcpy(buf, bh->b_data + offset, len);
    brelse(bh);
    return 0;
}

static int write_to_nizifs(nizifs_info_t *info, byte4_t block, byte4_t offset, void *buf, byte4_t len) {
    byte4_t block_size = info->sb.block_size;
    byte4_t bd_block_size = info->vfs_sb->s_bdev->bd_block_size;

    struct buffer_head *bh;

    byte4_t abs = block * block_size + offset;
    block = abs / bd_block_size;
    offset = abs % bd_block_size;
    if (offset + len > bd_block_size)   // should never happen
        return -EINVAL;
    if (!(bh = sb_bread(info->vfs_sb, block)))
        return -EIO;
    memcpy(bh->b_data + offset, buf, len);
    mark_buffer_dirty(bh);  // TODO: will this happen imediately??
    brelse(bh);
    return 0;
}

/* Read a file entry from the underlying block device */
int read_entry_from_nizifs(nizifs_info_t *info, int ino, nizifs_file_entry_t *fe) {
    byte4_t len = sizeof(nizifs_file_entry_t);
    return read_from_nizifs(info, info->sb.entry_table_block_start, ino*len, fe, len);
}

/* Read a file entry using VFS inode number*/
int read_entry_with_vfs_ino(nizifs_info_t *info, int vfs_ino, nizifs_file_entry_t *fe) {
    return read_entry_from_nizifs(info, V2N_INODE_NUM(vfs_ino), fe);
}

/* Write a file entry to underlying block device */
int write_entry_to_nizifs(nizifs_info_t *info, int ino, nizifs_file_entry_t *fe) {
    byte4_t len = sizeof(nizifs_file_entry_t);
    return write_to_nizifs(info, info->sb.entry_table_block_start,  ino*len, fe, len);
}

/* Read our super block from the underlying block device */
int read_sb_from_nizifs(nizifs_info_t *info, nizifs_super_block_t *sb) {
    struct buffer_head *bh;

    if (!(bh = sb_bread(info->vfs_sb, 0)) ) {   // super block is the 0th block
        return -EIO;
    }
    memcpy(sb, bh->b_data, NIZI_FS_BLOCK_SIZE);
    brelse(bh);
    return 0;
}

/* Unset used blocks bit  */
static void nizifs_unset_data_block(nizifs_info_t *info, int i) {
    // TODO: Here's a global lock
    spin_lock(&info->lock);
    info->used_blocks[i] = 0;
    spin_unlock(&info->lock);
}

int nizifs_update(nizifs_info_t *info, int vfs_ino, int *size, int *timestamp, int *perms) {
    nizifs_file_entry_t fe;
    int i, retval;

    if ((retval = read_entry_with_vfs_ino(info, vfs_ino, &fe)) < 0)
        return retval;
    if (size) fe.size = *size;
    if (timestamp) fe.timestamp = *timestamp;
    if (perms && (*perms <= 07)) fe.perms = *perms;

    // TODO: why i start from this?
    for (i = (fe.size+info->sb.block_size-1) / info->sb.block_size; i < NIZI_FS_DATA_BLOCK_CNT; i++) {
        if (fe.blocks[i]) {
            nizifs_unset_data_block(info, fe.blocks[i]);
            fe.blocks[i] = 0;
        }
    }

    return write_entry_to_nizifs(info, V2N_INODE_NUM(vfs_ino), &fe);
}



