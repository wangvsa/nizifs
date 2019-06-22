#include "nizifs.h"
#include <linux/version.h>

static int nizifs_file_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "nizifs: nizifs_file_release\n");
    return 0;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,11,0))
static int nizifs_readdir(struct file *file, void *dirent, filldir_t filldir) {
    struct dentry *de = file->f_dentry;

    int ret;

    printk(KERN_INFO "nizifs: nizifs_readdir: %Ld\n", file->f_pos);

    // curent directory "." at position 0
    if (file->f_pos == 0) {
        ret = filldir(dirent, ".", 1, file->f_pos, de->d_inode->i_no, DT_DIR);
        if (ret)
            return ret
        file->f_pos++;
    }
    // parent directory ".." at position 1
    if (file->f_pos == 1) {
        ret = filldir(dirent, "..", 2, file->f_pos, de->d_inode->i_no, DT_DIR);
        if (ret)
            return ret;
        file->f_pos++;
    }

    loff_t pos = 1;
    nizifs_info_t *info = de->d_inode->i_sb->s_fs_info;
    for (int ino = 0; ino < info->sb.entry_count; ino++) {

    }
    return 0;
}
#else
static int nizifs_iterate(struct file *file, struct dir_context *ctx) {
    printk(KERN_INFO "nizifs: nizifs_iterate: %Ld\n", ctx->pos);

    if (!dir_emit_dots(file, ctx))
        return -ENOSPC;

    nizifs_info_t *info = file_inode(file)->i_sb->s_fs_info;
    return 0;
    //return nizifs_list(info, file, ctx);
}
#endif


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
