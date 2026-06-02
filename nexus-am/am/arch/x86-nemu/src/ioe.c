#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48   // Note that this is not standard
static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  return inl(RTC_PORT) - boot_time;
}


uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  int i, j;
  for (j = 0; j < h; j++) {
    for (i = 0; i < w; i++) {
      fb[(y + j) * _screen.width + (x + i)] = pixels[j * w + i];
    }
  }
}


void _draw_sync() {
}

#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64
#define KBD_STATUS_HASKEY 0x1

int _read_key() {
  if ((inb(KBD_STATUS_PORT) & KBD_STATUS_HASKEY) == 0) {
    return _KEY_NONE;
  }
  return inl(KBD_DATA_PORT);
}
