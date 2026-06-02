#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x4000000)
#define INIT_FILE "/etc/init"

uintptr_t loader(_Protect *as, const char *filename) {
  char init[128];

  if (filename == NULL) {
    int fd = fs_open(INIT_FILE, 0, 0);
    size_t len = fs_read(fd, init, sizeof(init) - 1);
    fs_close(fd);

    init[len] = '\0';
    filename = init;
    if (init[0] == '#' && init[1] == '!') {
      filename = init + 2;
    }

    for (char *p = (char *)filename; *p != '\0'; p ++) {
      if (*p == '\n' || *p == '\r') {
        *p = '\0';
        break;
      }
    }
  }

  int fd = fs_open(filename, 0, 0);
  size_t size = fs_lseek(fd, 0, SEEK_END);
  fs_lseek(fd, 0, SEEK_SET);
  fs_read(fd, DEFAULT_ENTRY, size);
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
