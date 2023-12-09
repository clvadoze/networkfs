#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

#define TOKEN f22aea6a-152e-4df0-bffb-2009965129c6

//some

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ivanov Ivan");
MODULE_VERSION("0.01");

int networkfs_init(void) {
  printk(KERN_INFO "Hello, World!\n");
  return 0;
}

void networkfs_exit(void) { printk(KERN_INFO "Goodbye!\n"); }

module_init(networkfs_init);
module_exit(networkfs_exit);
