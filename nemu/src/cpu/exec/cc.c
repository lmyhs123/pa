#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t* dest, uint8_t subcode) {
  switch (subcode) {
    case 0x4:
      rtl_get_ZF(dest);
      break;
    case 0x5:
      rtl_get_ZF(dest);
      rtl_xori(dest, dest, 1);
      break;
    default:
      TODO();
  }
}
