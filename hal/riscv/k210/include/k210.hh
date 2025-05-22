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
		#define PLIC_MPRIORITY(hart)    (PLIC_V + 0x200000 + (hart) * 0x2000)
		#define PLIC_MCLAIM(hart)       (PLIC_V + 0x200004 + (hart) * 0x2000)

		#define GPIOHS                  0x38001000L
		#define DMAC                    0x50000000L
		#define GPIO                    0x50200000L
		#define SPI_SLAVE               0x50240000L
		#define FPIOA                   0x502B0000L
		#define SPI0                    0x52000000L
		#define SPI1                    0x53000000L
		#define SPI2                    0x54000000L
		#define SYSCTL                  0x50440000L

		#define GPIOHS_V                (GPIOHS + VIRT_OFFSET)
		#define DMAC_V                  (DMAC + VIRT_OFFSET)
		#define GPIO_V                  (GPIO + VIRT_OFFSET)
		#define SPI_SLAVE_V             (SPI_SLAVE + VIRT_OFFSET)
		#define FPIOA_V                 (FPIOA + VIRT_OFFSET)
		#define SPI0_V                  (SPI0 + VIRT_OFFSET)
		#define SPI1_V                  (SPI1 + VIRT_OFFSET)
		#define SPI2_V                  (SPI2 + VIRT_OFFSET)
		#define SYSCTL_V                (SYSCTL + VIRT_OFFSET)

		// the physical address of rustsbi
		#define RUSTSBI_BASE            0x80000000L

		// the kernel expects there to be RAM
		// for use by the kernel and user pages
		// from physical address 0x80020000 to PHYSTOP.
		#define KERNBASE                0x80020000L

		#define PHYSTOP                 0x88000000L

		// map kernel stacks beneath the trampoline,
		// each surrounded by invalid guard pages.
		// #define KSTACK(p)               (TRAMPOLINE - ((p) + 1) * 2 * PGSIZE)
		#define VKSTACK                 0x3EC0000000L

		#define MAXUVA                  RUSTSBI_BASE

	} // namespace k210

} // namespace riscv