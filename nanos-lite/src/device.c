#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  static char event[32];
  static size_t event_len = 0;
  static size_t event_pos = 0;

  if (event_pos == event_len) {
    int key = _read_key();
    int n;

    if (key == _KEY_NONE) {
      n = snprintf(event, sizeof(event), "t %lu\n", _uptime());
    } else {
      const char *type = (key & 0x8000) ? "kd" : "ku";
      int keycode = key & ~0x8000;
      n = snprintf(event, sizeof(event), "%s %s\n", type, keyname[keycode]);
    }

    event_len = n < sizeof(event) ? n : sizeof(event) - 1;
    event_pos = 0;
  }

  size_t read_len = event_len - event_pos;
  if (read_len > len) {
    read_len = len;
  }

  memcpy(buf, event + event_pos, read_len);
  event_pos += read_len;
  return read_len;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  memcpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  const uint32_t *pixels = buf;
  size_t nr_pixels = len / sizeof(uint32_t);
  size_t pos = offset / sizeof(uint32_t);

  while (nr_pixels > 0) {
    int x = pos % _screen.width;
    int y = pos / _screen.width;
    int n = _screen.width - x;

    if ((size_t)n > nr_pixels) {
      n = nr_pixels;
    }

    _draw_rect((uint32_t *)pixels, x, y, n, 1);
    pixels += n;
    pos += n;
    nr_pixels -= n;
  }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n",
      _screen.width, _screen.height);
}
