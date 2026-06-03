#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  int key = _read_key();
  Log("events_read: key = 0x%x", key);
  if (key == _KEY_NONE) {
    return 0;
  }

  const char *type = (key & 0x8000) ? "kd" : "ku";
  int keycode = key & ~0x8000;
  int n = snprintf(buf, len, "%s %s\n", type, keyname[keycode]);
  return n < len ? n : len;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  int x = (offset / 4) % _screen.width;
  int y = (offset / 4) / _screen.width;
  _draw_rect(buf, x, y, len / 4, 1);
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n",
      _screen.width, _screen.height);
}
