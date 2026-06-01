#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

void ramdisk_read(void *buf, off_t offset, size_t len);
size_t get_ramdisk_size();
bool gdb_memcpy_to_qemu(uint32_t, void *, int);

uintptr_t loader(_Protect *as, const char *filename) {
  size_t size = get_ramdisk_size();
  ramdisk_read(DEFAULT_ENTRY, 0, size);
#ifdef DIFF_TEST
  gdb_memcpy_to_qemu((uint32_t)DEFAULT_ENTRY, DEFAULT_ENTRY, size);
#endif
  return (uintptr_t)DEFAULT_ENTRY;
}
