/* Nizi File System Module */
#include <linux/module.h>       /* For module related macros, ... */
#include <linux/kernel.h>       /* For printk, ... */
#include <linux/version.h>      /* For LINUX_VERSION_CODE & KERNEL_VERSION */
#include <linux/fs.h>           /* For system calls, structures, ... */
#include <linux/errno.h>        /* For error codes */

#include "nizifs.h"     /* For nizifs related defines, data structures, ... */

/*
 * Data declarations in VFS
 * Required to implement a filesystem
 */
static struct file_system_type nizifs;
static struct super_operations nizifs_sops;
static struct inode_operations nizifs_iops;
static struct file_operations nizifs_fops;
static struct address_space_operations nizifs_aops;


static struct inode *nizifs_root_inode;

/* Called when mount the filesystem */
static int nizifs_fill_super(struct super_block *sb, void *data, int silent)
{
	printk(KERN_INFO "nizifs: nizifs_fill_super\n");

	sb->s_blocksize = SIMULA_FS_BLOCK_SIZE;
	sb->s_blocksize_bits = SIMULA_FS_BLOCK_SIZE_BITS;
	sb->s_magic = SIMULA_FS_TYPE;
	sb->s_type = &nizifs;                   // file_system_type
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


static struct file_system_type nizifs =
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
