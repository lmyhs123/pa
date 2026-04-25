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
      case 0x7: {  // a / nbe
  rtl_get_CF(dest);
  rtl_xori(dest, dest, 1);
  rtl_get_ZF(&t0);
  rtl_xori(&t0, &t0, 1);
  rtl_and(dest, dest, &t0);
  break;
}

case 0x6: {  // be / na
  rtl_get_CF(dest);
  rtl_get_ZF(&t0);
  rtl_or(dest, dest, &t0);
  break;
}

case 0x8:  // s
  rtl_get_SF(dest);
  break;


  case 0x9:  // ns
  rtl_get_SF(dest);
  rtl_xori(dest, dest, 1);
  break;

    default:
   
  }
}
