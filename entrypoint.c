// #include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define TOKEN "f22aea6a-152e-4df0-bffb-2009965129c6"
#define TOKEN_PATTERN "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matson Artem");
MODULE_VERSION("0.01");

struct dentry *networkfs_lookup(struct inode *parent, struct dentry *child,
                                unsigned int flag) {
  return NULL;
}

struct inode_operations networkfs_inode_ops = {
    .lookup = &networkfs_lookup,
};

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;

  loff_t record_counter = 0;

  while (true) {
    switch (ctx->pos) {
      case 0:
        dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR);
        break;

      case 1:
        struct inode *parent_inode = dentry->d_parent->d_inode;
        dir_emit(ctx, "..", 2, parent_inode->i_ino, DT_DIR);
        break;

      case 2:
        dir_emit(ctx, "test.txt", strlen("test.txt"), 1001, DT_REG);
        break;

      default:
        return record_counter;
    }///

    ++record_counter;
    ++ctx->pos;
  }
}

struct file_operations networkfs_dir_ops = {
    .iterate = networkfs_iterate,
};

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *parent, umode_t mode,
                                  int i_ino) {
  struct inode *inode;
  inode = new_inode(sb);

  if (inode != NULL) {
    inode->i_ino = i_ino;
    inode->i_op = &networkfs_inode_ops;
    inode->i_fop = &networkfs_dir_ops;
    inode_init_owner(&init_user_ns, inode, parent, mode);
  }

  return inode;
}

int networkfs_fill_super(struct super_block *sb, struct fs_context *fc) {
  struct inode *inode = networkfs_get_inode(sb, NULL, S_IFDIR, 1000);
  sb->s_root = d_make_root(inode);

  if (sb->s_root == NULL) {
    return -ENOMEM;
  }
  sb->s_fs_info = kmalloc(sizeof(TOKEN_PATTERN), GFP_KERNEL);
  if (sb->s_fs_info == NULL) {
    return -ENOMEM;
  }
  strcpy(sb->s_fs_info, fc->source);

  return 0;
}

int networkfs_get_tree(struct fs_context *fc) {
  int ret = get_tree_nodev(fc, &networkfs_fill_super);

  if (ret != 0) {
    printk(KERN_ERR "networkfs: unable to mount: error code %d", ret);
  }

  return ret;
}

struct fs_context_operations networkfs_context_ops = {.get_tree =
                                                          &networkfs_get_tree};

int networkfs_init_fs_context(struct fs_context *fc) {
  fc->ops = &networkfs_context_ops;
  return 0;
}

void networkfs_kill_sb(struct super_block *sb) {
  printk(KERN_INFO "%s\n", (char *)sb->s_fs_info);
  kfree(sb->s_fs_info);
  printk(KERN_INFO "networkfs: superblock is destroyed");
}

struct file_system_type networkfs_fs_type = {
    .name = "networkfs",
    .init_fs_context = &networkfs_init_fs_context,
    .kill_sb = &networkfs_kill_sb};

int networkfs_init(void) {
  int ret = register_filesystem(&networkfs_fs_type);
  if (ret != 0) {
    return ret;
  }
  printk(KERN_INFO "Init fs\n");
  return 0;
}

void networkfs_exit(void) {
  int ret = unregister_filesystem(&networkfs_fs_type);
  if (ret != 0) {
    printk(KERN_ERR "networkfs: error in unregister: error code %d", ret);
  }
  printk(KERN_INFO "Exit fs\n");
}

module_init(networkfs_init);
module_exit(networkfs_exit);
