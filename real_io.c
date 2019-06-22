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

int read_entry_from_nizifs(nizifs_info_t *info, int ino, nizifs_file_entry_t *fe) {
    byte4_t len = sizeof(nizifs_file_entry_t);
    return read_from_nizifs(info, info->sb.entry_table_block_start, ino*len, fe, len);
}

int read_sb_from_nizifs(nizifs_info_t *info, nizifs_super_block_t *sb) {
    struct buffer_head *bh;

    if (!(bh = sb_bread(info->vfs_sb, 0)) ) {   // super block is the 0th block
        return -EIO;
    }
    memcpy(sb, bh->b_data, NIZI_FS_BLOCK_SIZE);
    brelse(bh);
    return 0;
}
