#include <linux/fs.h>
#include <linux/fs_context.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include "http.h"
#include "models.h"

#define TOKEN_PATTERN "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Matson Artem");
MODULE_VERSION("0.01");

struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *parent, umode_t mode,
                                  int i_ino);

struct dentry *networkfs_lookup(struct inode *parent, struct dentry *child,
                                unsigned int flag) {
  const char *name = child->d_name.name;
  const char *token = parent->i_sb->s_fs_info;
  uint64_t ret;
  ALLOC_BUF(struct entry_info)
  ALLOC_INO
  sprintf(ino_ascii, "%lu", parent->i_ino);
  ret = networkfs_http_call(token, "lookup", (char *)buffer, buffer_size, 2,
                            "parent", ino_ascii, "name", name);
  if (ret != 0) {
    goto free;
  }

  umode_t mode;
  switch (buffer->entry_type) {
    case DT_DIR:
      mode = S_IFDIR;
      break;
    case DT_REG:
      mode = S_IFREG;
      break;
    default:
      goto free;
  }

  struct inode *inode =
      networkfs_get_inode(parent->i_sb, NULL, mode | S_IRWXUGO, buffer->ino);
  d_add(child, inode);

free:
  FREE_INO
  FREE_BUF
  return NULL;
}

int networkfs_unlink(struct inode *parent, struct dentry *child) {
  const char *name = child->d_name.name;
  const char *token = parent->i_sb->s_fs_info;
  uint64_t ret;
  ALLOC_INO
  sprintf(ino_ascii, "%lu", parent->i_ino);
  ret = networkfs_http_call(token, "unlink", NULL, 0, 2,
                            "parent", ino_ascii, "name", name);

  FREE_INO
  return ret;
}
int networkfs_create(struct user_namespace *user_ns, struct inode *parent,
                     struct dentry *child, umode_t mode, bool b) {
  const char *name = child->d_name.name;
  const char *token = parent->i_sb->s_fs_info;
  uint64_t ret;
  ALLOC_BUF(ino_t);
  ALLOC_INO
  sprintf(ino_ascii, "%lu", parent->i_ino);
  ret = networkfs_http_call(token, "create", (char *)buffer, buffer_size, 3,
                            "parent", ino_ascii, "name", name, "type", "file");
  if (ret != 0) {
    goto free;
  }
  struct inode* new_inode = networkfs_get_inode(parent->i_sb, NULL, mode | S_IFREG | S_IRWXUGO, *buffer);
  if (new_inode == NULL) {
    goto free;
  }
  d_add(child, new_inode);

free:
  FREE_BUF
  FREE_INO
  return ret;
}

struct inode_operations networkfs_inode_ops = {.lookup = &networkfs_lookup,
                                               .create = &networkfs_create,
                                               .unlink = &networkfs_unlink};

int networkfs_iterate(struct file *filp, struct dir_context *ctx) {
  struct dentry *dentry = filp->f_path.dentry;
  struct inode *inode = dentry->d_inode;
  const char *token = inode->i_sb->s_fs_info;
  struct entry *current_entry;
  int64_t ret;

  ALLOC_BUF(struct entries)
  ALLOC_INO
  sprintf(ino_ascii, "%lu", inode->i_ino);
  ret = networkfs_http_call(token, "list", (char *)buffer, buffer_size, 1,
                            "inode", ino_ascii);
  if (ret != 0) {
    goto free;
  }
  loff_t start_cnt = ctx->pos;
  size_t files_cnt = buffer->entries_count + 2;
  while (ctx->pos < files_cnt) {
    switch (ctx->pos) {
      case 0:
        dir_emit(ctx, ".", 1, inode->i_ino, DT_DIR);
        break;
      case 1:
        dir_emit(ctx, "..", 2, dentry->d_parent->d_inode->i_ino, DT_DIR);
        break;
      default:
        current_entry = buffer->entries + ctx->pos - 2;
        dir_emit(ctx, current_entry->name, strlen(current_entry->name),
                 current_entry->ino, current_entry->entry_type);
    }
    ++ctx->pos;
  }
  ret = files_cnt - start_cnt;

free:
  FREE_INO
  FREE_BUF
  return ret;
}

struct file_operations networkfs_dir_ops = {
    .iterate = &networkfs_iterate,
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
  struct inode *inode =
      networkfs_get_inode(sb, NULL, S_IFDIR | S_IRWXUGO, 1000);
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
