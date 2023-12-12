struct inode *networkfs_get_inode(struct super_block *sb,
                                  const struct inode *parent, umode_t mode,
                                  int i_ino);

int networkfs_fill_super(struct super_block *sb, struct fs_context *fc);

int networkfs_get_tree(struct fs_context *fc);

int networkfs_init_fs_context(struct fs_context *fc);

void networkfs_kill_sb(struct super_block *sb);

int networkfs_iterate(struct file *filp, struct dir_context *ctx);

struct dentry *networkfs_lookup(struct inode *parent, struct dentry *child,
                                unsigned int flag);

int networkfs_unlink(struct inode *parent, struct dentry *child);

int networkfs_rmdir(struct inode *parent, struct dentry *child);

int networkfs_create(struct user_namespace *user_ns, struct inode *parent,
                     struct dentry *child, umode_t mode, bool b);

int networkfs_mkdir(struct user_namespace *user_ns, struct inode *parent,
                    struct dentry *child, umode_t mode);

int networkfs_iterate(struct file *filp, struct dir_context *ctx);

int networkfs_init(void);

void networkfs_exit(void);

struct fs_context_operations networkfs_context_ops = {.get_tree =
                                                          &networkfs_get_tree};

struct file_system_type networkfs_fs_type = {
    .name = "networkfs",
    .init_fs_context = &networkfs_init_fs_context,
    .kill_sb = &networkfs_kill_sb};

struct file_operations networkfs_dir_ops = {
    .iterate = &networkfs_iterate,
};

struct inode_operations networkfs_inode_ops = {.lookup = &networkfs_lookup,
                                               .create = &networkfs_create,
                                               .unlink = &networkfs_unlink,
                                               .mkdir = &networkfs_mkdir,
                                               .rmdir = &networkfs_rmdir};
