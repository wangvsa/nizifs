/* Nizi File System Module */
#include <linux/module.h>       /* For module related macros, ... */
#include <linux/kernel.h>       /* For printk, ... */
#include <linux/version.h>      /* For LINUX_VERSION_CODE & KERNEL_VERSION */
#include <linux/fs.h>           /* For system calls, structures, ... */
#include <linux/errno.h>        /* For error codes */

#include "nizifs.h"             /* For nizifs related defines, data structures, ... */
#include "real_io.h"            /* direct access to the underlying block device */


static struct inode *nizifs_root_inode;


int init_nizifs_info(nizifs_info_t *info) {

    int retval, i, j;

    // fill in our self super block
    if ((retval = read_sb_from_nizifs(info, &info->sb)) < 0)
        return retval;

    if (info->sb.type != NIZI_FS_TYPE) {
        printk(KERN_ERR "Wrong magic number, this is not a nizifs partition.\n");
        return -EINVAL;
    }

    // Mark used blocks
    byte1_t *used_blocks = (byte1_t *)(vmalloc(info->sb.partition_size));
    if (!used_blocks)
        return -ENOMEM;
    for (i = 0; i < info->sb.data_block_start; i++)
        used_blocks[i] = 1;
    for (i = info->sb.data_block_start; i < info->sb.partition_size; i++)
        used_blocks[i] = 0;

    nizifs_file_entry_t fe; // no need to keep after this function, so not a pointer
    for (i = 0; i < info->sb.entry_count; i++) {
        if( (retval = read_entry_from_nizifs(info, i, &fe) < 0 )) {
            vfree(used_blocks); // some thing wrong, need to free used_blocks and exit;
            return retval;
        }
        if (!fe.name[0]) continue;
        for(j = 0; j < NIZI_FS_DATA_BLOCK_CNT; j++) {
            if (fe.blocks[j] == 0) break;
            used_blocks[fe.blocks[j]] = 1;
        }
    }

    info->used_blocks = used_blocks;
    info->vfs_sb->s_fs_info = info;
    spin_lock_init(&info->lock);
    return 0;
}



/* Called when mount the filesystem */
static int nizifs_fill_super(struct super_block *sb, void *data, int silent)
{
	printk(KERN_INFO "nizifs: nizifs_fill_super\n");

	sb->s_type = &nizifs;                   // file_system_type
	sb->s_blocksize = NIZI_FS_BLOCK_SIZE;
	sb->s_blocksize_bits = NIZI_FS_BLOCK_SIZE_BITS;
	sb->s_magic = NIZI_FS_TYPE;
	sb->s_op = &nizifs_sops;                // super block operations

	nizifs_root_inode = iget_locked(sb, 1); // obtain an inode from VFS
	if (!nizifs_root_inode)
	{
		return -EACCES;
	}
	if (nizifs_root_inode->i_state & I_NEW) // allocated fresh now
	{
		printk(KERN_INFO "nizifs: Got new root inode, let's fill in\n");
		nizifs_root_inode->i_op = &nizifs_iops; // inode operations
		nizifs_root_inode->i_mode = S_IFDIR | S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
		nizifs_root_inode->i_fop = &nizifs_fops; // file operations
		nizifs_root_inode->i_mapping->a_ops = &nizifs_aops; // address operations
		unlock_new_inode(nizifs_root_inode);
	}
	else
	{
		printk(KERN_INFO "nizifs: Got root inode from inode cache\n");
	}

    #if (LINUX_VERSION_CODE < KERNEL_VERSION(3,4,0))
	sb->s_root = d_alloc_root(nizifs_root_inode);
    #else
	sb->s_root = d_make_root(nizifs_root_inode);
    #endif
	if (!sb->s_root)
	{
		iget_failed(nizifs_root_inode);
		return -ENOMEM;
	}

	return 0;
}


/*
 * Initialize the filesystem
 * Called when users call "mount" command
 */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
static int nizifs_get_sb(struct file_system_type *fs_type, int flags, const char *devname, void *data, struct vfsmount *vm)
{
	printk(KERN_INFO "nizifs: devname = %s\n", devname);
	 /* nizifs_fill_super() will be called to fill the super block */
	return get_sb_bdev(fs_type, flags, devname, data, &nizifs_fill_super, vm);
}
#else
static struct dentry *nizifs_mount(struct file_system_type *fs_type, int flags, const char *devname, void *data)
{
	printk(KERN_INFO "nizifs: devname = %s\n", devname);
	 /* nizifs_fill_super() will be called to fill the super block */
	return mount_bdev(fs_type, flags, devname, data, &nizifs_fill_super);
}
#endif


struct file_system_type nizifs =
{
	name: "nizifs", /* Name of our file system */
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,38))
	get_sb:  nizifs_get_sb,
    #else
	mount:  nizifs_mount,
    #endif
	kill_sb: kill_block_super,
	owner: THIS_MODULE
};

static int __init nizifs_init(void)
{
	int err = register_filesystem(&nizifs);
	return err;
}

static void __exit nizifs_exit(void)
{
	unregister_filesystem(&nizifs);
}

module_init(nizifs_init);
module_exit(nizifs_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Chen Wang <wang116@llnl.gov>");
MODULE_DESCRIPTION("nizifs - a posix resistant filesystem");
