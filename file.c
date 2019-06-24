#include <linux/version.h>
#include <linux/buffer_head.h>  /* map_bh, block_write_begin, block_write_full_page, generic_write_end */
#include <linux/mpage.h> /* mpage_readpage, ... */
#include "nizifs.h"
#include "real_io.h"

static int nizifs_file_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "nizifs: nizifs_file_release\n");
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
static int nizifs_readdir(struct file *file, void *dirent, filldir_t filldir) {
    printk(KERN_INFO "nizifs: nizifs_readdir: %Ld\n", file->f_pos);

    struct dentry *de = file->f_dentry;
    int retval;

    // curent directory "." at position 0
    if (file->f_pos == 0) {
        retval = filldir(dirent, ".", 1, file->f_pos, de->d_inode->i_no, DT_DIR);
        if (retval)
            return retval;
        file->f_pos++;
    }
    // parent directory ".." at position 1
    if (file->f_pos == 1) {
        retval = filldir(dirent, "..", 2, file->f_pos, de->d_inode->i_no, DT_DIR);
        if (retval)
            return retval;
        file->f_pos++;
    }

    // TODO: Need to understand this
    loff_t pos = 1;
    nizifs_file_entry_t fe;
    nizifs_info_t *info = de->d_inode->i_sb->s_fs_info;
    for (int ino = 0; ino < info->sb.entry_count; ino++) {
        if ((retval = read_entry_from_nizifs(info, ino, &fe)) < 0)
            return retval;
        if (!fe.name[0]) continue;
        pos++;
        if (file->f_pos == pos) {
            retval = filldir(dirent, fe.name, strlen(fe.name), file->f_pos, N2V_INODE_NUM(ino), DT_REG);
            if (retval)
                return retval;
            file->f_pos++;
        }
    }
    return 0;
}
#else
static int nizifs_iterate(struct file *file, struct dir_context *ctx) {
    int retval, ino;
    loff_t pos = 1;
    nizifs_file_entry_t fe;
    nizifs_info_t *info = file_inode(file)->i_sb->s_fs_info;

    printk(KERN_INFO "nizifs: nizifs_iterate: %Ld\n", ctx->pos);

    if (!dir_emit_dots(file, ctx))
        return -ENOSPC;

    for (ino = 0; ino < info->sb.entry_count; ino++) {
        if ((retval = read_entry_from_nizifs(info, ino, &fe)) < 0)
            return retval;
        if (!fe.name[0]) continue;
        pos++;
        if (ctx->pos == pos) {
            if (!dir_emit(ctx, fe.name, strlen(fe.name), N2V_INODE_NUM(ino), DT_REG))
                return -ENOSPC;
            file->f_pos++;
        }
    }
    return 0;
}
#endif

/* Return a unused data block index */
static int nizifs_get_data_block(nizifs_info_t *info) {
    int i;
    spin_lock(&info->lock);
    for (i = info->sb.data_block_start; i < info->sb.partition_size; i++) {
        if (info->used_blocks[i] == 0) {
            info->used_blocks[i] = 1;
            spin_unlock(&info->lock);
            return i;
        }
    }
    spin_unlock(&info->lock);
    return INV_BLOCK;
}

// TODO: Need to understand this and how it works with address_space_operations
static int nizifs_get_block(struct inode *inode, sector_t iblock, struct buffer_head *bh_result, int create) {
    struct super_block *sb = inode->i_sb;
    nizifs_info_t *info = (nizifs_info_t *)(sb->s_fs_info);
    nizifs_file_entry_t fe;
    sector_t phys;      // TODO: What's this
    int retval;

    printk(KERN_INFO "nizifs: nizifs_get_block called for I: %ld, B: %llu, C: %d\n",
            inode->i_ino, (unsigned long long)(iblock), create);
    if (iblock >= NIZI_FS_DATA_BLOCK_CNT)
        return -ENOSPC;
    if ((retval = read_entry_with_vfs_ino(info, inode->i_ino, &fe)) < 0)
        return retval;
    if (!fe.blocks[iblock]) {
        if (!create) {
            return -EIO;
        } else {
            if ((fe.blocks[iblock] = nizifs_get_data_block(info)) == INV_BLOCK)
                return -ENOSPC;
            if ((retval = nizifs_update_file_entry(info, inode->i_ino, &fe)) < 0)
                return retval;
        }
    }

    phys = fe.blocks[iblock];
    map_bh(bh_result, sb, phys);    // TODO: Understand this
    return 0;
}



static int nizifs_readpage(struct file *file, struct page *page) {
    printk(KERN_INFO "nizifs: nizifs_readpage\n");
    return mpage_readpage(page, nizifs_get_block);
}
static int nizifs_writepage(struct page *page, struct writeback_control *wbc) {
    printk(KERN_INFO "nizifs: nizifs_writepage\n");
    return block_write_full_page(page, nizifs_get_block, wbc);
}
static int nizifs_write_begin(struct file *file, struct address_space *mapping,
        loff_t pos, unsigned len, unsigned flags, struct page **pagep, void **fsdaata) {
    printk(KERN_INFO "nizifs: nizifs_write_begin\n");
    *pagep = NULL;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2, 6, 36))
    return block_write_begin(file, mapping, pos, len, flags, pagep, fsdata, nizifs_get_block);
#else
    return block_write_begin(mapping, pos, len, flags, pagep, nizifs_get_block);
#endif
}

const struct file_operations nizifs_fops = {
    open: generic_file_open,
    release: nizifs_file_release,
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(3,16,0))
    read: do_sync_read,
    write: do_sync_write,
    aio_read: generic_file_aio_read,
    aio_write: generic_file_aio_write,
    #elif (LINUX_VERSION_CODE < KERNEL_VERSION(4,1,0))
    read: new_sync_read,
    write: new_sync_write,
    read_iter: generic_file_read_iter,
    write_iter: generic_file_write_iter,
    #else
    read_iter: generic_file_read_iter,
    write_iter: generic_file_write_iter,
    #endif

    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
    fsync: simple_sync_file
    #else
    fsync: noop_fsync
    #endif
};

const struct file_operations nizifs_dops = {
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
    readdir: nizifs_readdir
    #else
    iterate: nizifs_iterate
    #endif
};

const struct address_space_operations nizifs_aops = {
    readpage: nizifs_readpage,
    writepage: nizifs_writepage,
    write_begin: nizifs_write_begin,
    write_end: generic_write_end
};
