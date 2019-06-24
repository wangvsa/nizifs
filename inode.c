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

// TODO: Need to understand this, especially dentry
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,6,0))
static struct dentry *nizifs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, struct nameidata *nameidata)
#else
static struct dentry *nizifs_inode_lookup(struct inode *parent_inode, struct dentry *dentry, unsigned intflags)
#endif
{
    nizifs_info_t *info = (nizifs_info_t *)(parent_inode->i_sb->s_fs_info);
    char fn[dentry->d_name.len + 1];
    int ino;
    nizifs_file_entry_t fe;
    struct inode *file_inode = NULL;

    printk(KERN_INFO "nizifs: nizifs_inode_lookup\n");

    if (parent_inode->i_ino != nizifs_root_inode->i_ino)
        return ERR_PTR(-ENOENT);
    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;
    if ((ino = nizifs_lookup_file(info, fn, &fe)) == INV_INODE)
        return d_splice_alias(file_inode, dentry);    // Possibly create a new one

    file_inode = iget_locked(parent_inode->i_sb, ino);
    if (!file_inode)
        return ERR_PTR(-EACCES);
    if (file_inode->i_state & I_NEW) {
        printk(KERN_INFO "nizifs: Got new VFS inode for #%d\n", ino);
        file_inode->i_size = fe.size;
        file_inode->i_mode = S_IFREG;
        //file_inode->i_mode |= ((fe.perms & 4) ? S_IRUSR|S_IRGRP|S_IROTH : 0);
        //file_inode->i_mode |= ((fe.perms & 2) ? S_IWUSR|S_IWGRP|S_IWOTH : 0);
        //file_inode->i_mode |= ((fe.perms & 1) ? S_IXUSR|S_IXGRP|S_IXOTH : 0);

        file_inode->i_mode |= (S_IRUSR|S_IRGRP|S_IROTH|S_IWUSR|S_IWGRP|S_IWOTH|S_IXUSR|S_IXGRP|S_IXOTH);

        file_inode->i_mapping->a_ops = &nizifs_aops;
        file_inode->i_fop = &nizifs_fops;
        unlock_new_inode(file_inode);
    } else {
        printk(KERN_INFO "nizifs: Got VFS inode from inode cache");
    }
    d_add(dentry, file_inode);
    return NULL;
}

static int nizifs_inode_unlink(struct inode *parent_inode, struct dentry *dentry) {

    nizifs_info_t *info = (nizifs_info_t *)(parent_inode->i_sb->s_fs_info);
    char fn[dentry->d_name.len + 1];
    int ino;
    struct inode *file_inode = dentry->d_inode;

    printk(KERN_INFO "nizifs: nizifs_inode_unlink\n");

    strncpy(fn, dentry->d_name.name, dentry->d_name.len);
    fn[dentry->d_name.len] = 0;

    if ((ino = nizifs_remove_file(info, fn)) == INV_INODE)
        return -EINVAL;

    inode_dec_link_count(file_inode);
    return 0;
}

const struct inode_operations nizifs_iops = {
    create: nizifs_inode_create,        /* called by the open(2) and creat(2) system calls */
    unlink: nizifs_inode_unlink,
    lookup: nizifs_inode_lookup
};
