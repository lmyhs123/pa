#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */
  uint32_t eflags = 0;
  eflags |= cpu.eflags.CF;
  eflags |= cpu.eflags.ZF << 6;
  eflags |= cpu.eflags.SF << 7;
  eflags |= cpu.eflags.OF << 11;

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, eflags);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, cpu.cs);

  cpu.esp -= 4;
  vaddr_write(cpu.esp, 4, ret_addr);

  vaddr_t addr = cpu.idtr.base + NO * 8;
  uint32_t low = vaddr_read(addr, 4);
  uint32_t high = vaddr_read(addr + 4, 4);

  cpu.cs = (low >> 16) & 0xffff;
  decoding.jmp_eip = (high & 0xffff0000) | (low & 0x0000ffff);
}

void dev_raise_intr() {
}
