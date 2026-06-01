#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t* dest, uint8_t subcode) {
  switch (subcode) {
    case 0x2: {  // b / c / nae
      rtl_get_CF(dest);
      break;
    }

    case 0x3: {  // nb / nc / ae
      rtl_get_CF(dest);
      rtl_xori(dest, dest, 1);
      break;
    }

    case 0x4:  // e / z
      rtl_get_ZF(dest);
      break;

    case 0x5:  // ne / nz
      rtl_get_ZF(dest);
      rtl_xori(dest, dest, 1);
      break;

    case 0x6: {  // be / na
      rtl_get_CF(dest);
      rtl_get_ZF(&t0);
      rtl_or(dest, dest, &t0);
      break;
    }

    case 0x7: {  // a / nbe
      rtl_get_CF(dest);
      rtl_xori(dest, dest, 1);
      rtl_get_ZF(&t0);
      rtl_xori(&t0, &t0, 1);
      rtl_and(dest, dest, &t0);
      break;
    }

    case 0x8:  // s
      rtl_get_SF(dest);
      break;

    case 0x9:  // ns
      rtl_get_SF(dest);
      rtl_xori(dest, dest, 1);
      break;

    case 0xc: {  // l / nge
      rtl_get_SF(dest);
      rtl_get_OF(&t0);
      rtl_xor(dest, dest, &t0);
      break;
    }

    case 0xd: {  // ge / nl
      rtl_get_SF(dest);
      rtl_get_OF(&t0);
      rtl_xor(dest, dest, &t0);
      rtl_xori(dest, dest, 1);
      break;
    }

    case 0xe: {  // le / ng
      rtl_get_SF(dest);
      rtl_get_OF(&t0);
      rtl_xor(dest, dest, &t0);   // SF != OF
      rtl_get_ZF(&t1);
      rtl_or(dest, dest, &t1);    // ZF || (SF != OF)
      break;
    }

    case 0xf: {  // g / nle
      rtl_get_SF(dest);
      rtl_get_OF(&t0);
      rtl_xor(dest, dest, &t0);   // SF != OF
      rtl_xori(dest, dest, 1);    // SF == OF
      rtl_get_ZF(&t1);
      rtl_xori(&t1, &t1, 1);      // !ZF
      rtl_and(dest, dest, &t1);   // !ZF && (SF == OF)
      break;
    }

    default:
      TODO();
  }
}
