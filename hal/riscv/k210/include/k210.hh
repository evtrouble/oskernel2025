#pragma once

#include "riscv.hh"

namespace riscv
{
	namespace k210
	{
		// Physical memory layout

		// k210 peripherals
		// (0x0200_0000, 0x1000),      /* CLINT     */
		// // we only need claim/complete for target0 after initializing
		// (0x0C20_0000, 0x1000),      /* PLIC      */
		// (0x3800_0000, 0x1000),      /* UARTHS    */
		// (0x3800_1000, 0x1000),      /* GPIOHS    */
		// (0x5020_0000, 0x1000),      /* GPIO      */
		// (0x5024_0000, 0x1000),      /* SPI_SLAVE */
		// (0x502B_0000, 0x1000),      /* FPIOA     */
		// (0x502D_0000, 0x1000),      /* TIMER0    */
		// (0x502E_0000, 0x1000),      /* TIMER1    */
		// (0x502F_0000, 0x1000),      /* TIMER2    */
		// (0x5044_0000, 0x1000),      /* SYSCTL    */
		// (0x5200_0000, 0x1000),      /* SPI0      */
		// (0x5300_0000, 0x1000),      /* SPI1      */
		// (0x5400_0000, 0x1000),      /* SPI2      */
		// (0x8000_0000, 0x600000),    /* Memory    */

		// qemu -machine virt is set up like this,
		// based on qemu's hw/riscv/virt.c:
		//
		// 00001000 -- boot ROM, provided by qemu
		// 02000000 -- CLINT
		// 0C000000 -- PLIC
		// 10000000 -- uart0 
		// 10001000 -- virtio disk 
		// 80000000 -- boot ROM jumps here in machine mode
		//             -kernel loads the kernel here
		// unused RAM after 80000000.

		#define VIRT_OFFSET             0x3F00000000L

		#define UART                    0x38000000L

		#define UART_V                  (UART + VIRT_OFFSET)

		// local interrupt controller, which contains the timer.
		#define CLINT                   0x02000000L
		#define CLINT_V                 (CLINT + VIRT_OFFSET)

		#define PLIC                    0x0c000000L
		#define PLIC_V                  (PLIC + VIRT_OFFSET)

		#define PLIC_PRIORITY           (PLIC_V + 0x0)
		#define PLIC_PENDING            (PLIC_V + 0x1000)
		#define PLIC_MENABLE(hart)      (PLIC_V + 0x2000 + (hart) * 0x100)
		#define PLIC_SENABLE(hart)      (PLIC_V + 0x2080 + (hart) * 0x100)
		#define PLIC_MPRIORITY(hart)    (PLIC_V + 0x200000 + (hart) * 0x2000)
		#define PLIC_SPRIORITY(hart)    (PLIC_V + 0x201000 + (hart) * 0x2000)
		#define PLIC_MCLAIM(hart)       (PLIC_V + 0x200004 + (hart) * 0x2000)
		#define PLIC_SCLAIM(hart)       (PLIC_V + 0x201004 + (hart) * 0x2000)

		#define GPIOHS                  0x38001000
		#define DMAC                    0x50000000
		#define GPIO                    0x50200000
		#define SPI_SLAVE               0x50240000
		#define FPIOA                   0x502B0000
		#define SPI0                    0x52000000
		#define SPI1                    0x53000000
		#define SPI2                    0x54000000
		#define SYSCTL                  0x50440000

		#define GPIOHS_V                (0x38001000 + VIRT_OFFSET)
		#define DMAC_V                  (0x50000000 + VIRT_OFFSET)
		#define GPIO_V                  (0x50200000 + VIRT_OFFSET)
		#define SPI_SLAVE_V             (0x50240000 + VIRT_OFFSET)
		#define FPIOA_V                 (0x502B0000 + VIRT_OFFSET)
		#define SPI0_V                  (0x52000000 + VIRT_OFFSET)
		#define SPI1_V                  (0x53000000 + VIRT_OFFSET)
		#define SPI2_V                  (0x54000000 + VIRT_OFFSET)
		#define SYSCTL_V                (0x50440000 + VIRT_OFFSET)

		// the physical address of rustsbi
		#define RUSTSBI_BASE            0x80000000

		// the kernel expects there to be RAM
		// for use by the kernel and user pages
		// from physical address 0x80020000 to PHYSTOP.
		#define KERNBASE                0x80020000

		#define PHYSTOP                 0x82000000

		// Sv48
		#define MAXVA (1L << (9 + 9 + 9 + 9 + 12 - 1))

		// map the trampoline page to the highest address,
		// in both user and kernel space.
		#define TRAMPOLINE              (MAXVA - PG_SIZE)

		// map kernel stacks beneath the trampoline,
		// each surrounded by invalid guard pages.
		// #define KSTACK(p)               (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)
		#define VKSTACK                 0x3EC0000000L

		// User memory layout.
		// Address zero first:
		//   text
		//   original data and bss
		//   fixed-size stack
		//   expandable heap
		//   ...
		//   TRAPFRAME (p->trapframe, used by the trampoline)
		//   TRAMPOLINE (the same page as in the kernel)
		#define TRAPFRAME               (TRAMPOLINE - PG_SIZE)

		#define MAXUVA                  RUSTSBI_BASE

	} // namespace k210

} // namespace riscv