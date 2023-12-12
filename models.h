#include <linux/types.h>

struct entry {
  unsigned char entry_type; // DT_DIR (4) or DT_REG (8)
  ino_t ino;
  char name[256];
};

struct entries {
  size_t entries_count;
  struct entry  entries[16];
};