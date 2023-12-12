#include <linux/types.h>

struct entry {
  unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
  ino_t ino;
  char name[256];
};

struct entries {
  size_t entries_count;
  struct entry entries[16];
};

struct entry_info {
  unsigned char entry_type;  // DT_DIR (4) or DT_REG (8)
  ino_t ino;
};

struct create_info {
  ino_t ino;
};

#define ALLOC_INO                                           \
  char *ino_ascii = kmalloc(sizeof(ino_t) + 1, GFP_KERNEL); \
  if (ino_ascii == NULL) {                                  \
    ret = -ENOMEM;                                          \
    goto ino_end;                                           \
  }

#define FREE_INO    \
  kfree(ino_ascii); \
  ino_end:

#define ALLOC_BUF(model)                            \
  size_t buffer_size = sizeof(model);               \
  model *buffer = kmalloc(buffer_size, GFP_KERNEL); \
  if (buffer == NULL) {                             \
    ret = -ENOMEM;                                  \
    goto buf_end;                                   \
  }

#define FREE_BUF \
  kfree(buffer); \
  buf_end:
