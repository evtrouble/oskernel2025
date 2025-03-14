#pragma once
#include "kernel/types.hh"
#include "uart/virtual_uart.hh"
#include "smp/spin_lock.hh"

namespace riscv
{
	void consoleinit(void);
    void consputc(int c);
    void consoleintr(int c);

} // namespace riscv
