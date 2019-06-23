#include <linux/version.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include "nizifs.h"
#include "real_io.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,3,0))
static int nizifs_inode_create(struct inode *parent_inode, struct dentry *dentry, int mode, struct nameidata *nameidata)
#elif (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
static int nizifs_inode_create(struct inode *parent_inode, struct dentry *dentry, umode_t mode, struct nameidata *nameidata)
#else
static int nizifs_inode_create(struct inode *parent_inode, struct dentry *dentry, umode_t mode, bool excel)
#endif
{
    char fn[dentry->d_name.len + 1];
    int perms = 0;
    nizifs_info_t *info = (nizifs_info_t *)(parent_inode->i_sb->s_fs_info);
    int ino;

    struct inode *file_inode;
    nizifs_file_entry_t fe;

    printk(KERN_INFO "nizifs: nizifs_inode_create\n");

    // Set file name
    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;

    // Set the permissions
    perms |= (mode & S_IRUSR) ? 4:0;
    perms |= (mode & S_IWUSR) ? 2:0;
    perms |= (mode & S_IXUSR) ? 1:0;

    // Create the file in our system
    if ((ino = nizifs_create_file(info, fn, perms, &fe)) == INV_INODE)
        return -ENOSPC;

    // Create the inode in VFS
    file_inode = new_inode(parent_inode->i_sb);
    if (!file_inode) {
        nizifs_remove_file(info, fn);
        return -ENOMEM;
    }
    file_inode->i_ino = ino;
    file_inode->i_size = fe.size;
    file_inode->i_mode = S_IFREG | mode;
    file_inode->i_mapping->a_ops = &nizifs_aops;
    file_inode->i_fop = &nizifs_fops;

    if(insert_inode_locked(file_inode) < 0) {
        make_bad_inode(file_inode);
        // TODO: what is this
        iput(file_inode);
        nizifs_remove_file(info, fn);
        return -EIO;
    }

    // TODO: what's this
    d_instantiate(dentry, file_inode);
    unlock_new_inode(file_inode);
    return 0;
}

const struct inode_operations nizifs_iops = {
    create: nizifs_inode_create
    //ulink: nizifs_inode_unlink,
    //lookup: nizifs_inode_lookup
};
