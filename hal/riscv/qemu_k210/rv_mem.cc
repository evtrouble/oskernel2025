//
// Created by Li Shuang ( pseudonym ) on 2024-07-10
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "rv_mem.hh"

#include <hsai_global.hh>
#include <hsai_log.hh>
#include <mem/page.hh>
#inlcude "mm/virtual_memory_manager.hh"

#include "rv_cpu.hh"
#include "riscv.hh"
#include "qemu_k210.hh"
#include "tlb_manager.hh"

extern "C" {
extern ulong xn6_end;
}

namespace riscv
{
	namespace qemuk210
	{
		static Memory k_k210_mem;

		void Memory::memory_init()
		{
			register_memory( &k_k210_mem );
		}

		ulong Memory::mem_start()
		{
			ulong end_addr	= (ulong) &xn6_end;
			end_addr	   += _1M - 1;
			end_addr	   &= ~( _1M - 1 );
			return end_addr;
		}
		ulong Memory::mem_size()
		{
			return memory::mem_size;
		}

		ulong Memory::to_vir( ulong addr ) { return addr; }

		ulong Memory::to_phy( ulong addr ) { return addr; }

		ulong Memory::to_io( ulong addr ) { return addr; }

		ulong Memory::to_dma( ulong addr ) { return addr; }

		void Memory::config_pt( ulong pt_addr )
		{
			// uart registers
			map_pages(*(PageTable*)pt_addr, UART_V, PG_SIZE, UART, PteEnum::pte_read_m | PteEnum::pte_write_m);
			
			#ifdef QEMU
			// virtio mmio disk interface
			map_pages(*(PageTable*)pt_addr, VIRTIO0_V, PG_SIZE, VIRTIO0, pte_read_m | pte_write_m);
			#endif
			// CLINT
			map_pages(*(PageTable*)pt_addr, CLINT_V, 0x10000, CLINT, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// PLIC
			map_pages(*(PageTable*)pt_addr, PLIC_V, 0x4000, PLIC, PteEnum::pte_read_m | PteEnum::pte_write_m);
			map_pages(*(PageTable*)pt_addr, PLIC_V + 0x4000, PGSIZE, PLIC + 0x200000, PteEnum::pte_read_m | PteEnum::pte_write_m);

			#ifndef QEMU
			// GPIOHS
			map_pages(*(PageTable*)pt_addr, GPIOHS_V, 0x1000, GPIOHS, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// DMAC
			// map_pages(*(PageTable*)pt_addr, DMAC_V, 0x1000, DMAC, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// GPIO
			// map_pages(*(PageTable*)pt_addr, GPIO_V, 0x1000, GPIO, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// SPI_SLAVE
			map_pages(*(PageTable*)pt_addr, SPI_SLAVE_V, 0x1000, SPI_SLAVE, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// FPIOA
			map_pages(*(PageTable*)pt_addr, FPIOA_V, 0x1000, FPIOA, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// SPI0
			map_pages(*(PageTable*)pt_addr, SPI0_V, 0x1000, SPI0, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// SPI1
			map_pages(*(PageTable*)pt_addr, SPI1_V, 0x1000, SPI1, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// SPI2
			map_pages(*(PageTable*)pt_addr, SPI2_V, 0x1000, SPI2, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// SYSCTL
			map_pages(*(PageTable*)pt_addr, SYSCTL_V, 0x1000, SYSCTL, PteEnum::pte_read_m | PteEnum::pte_write_m);
			#endif
			
			// map rustsbi
			// kvmmap(RUSTSBI_BASE, RUSTSBI_BASE, KERNBASE - RUSTSBI_BASE, PTE_R | PTE_X);
			// map kernel text executable and read-only.
			map_pages(*(PageTable*)pt_addr, KERNBASE, (uint64)etext - KERNBASE, KERNBASE, PteEnum::pte_read_m | PteEnum::pte_execute_m);
			// map kernel data and the physical RAM we'll make use of.
			map_pages(*(PageTable*)pt_addr, (uint64)etext, PHYSTOP - (uint64)etext, (uint64)etext, PteEnum::pte_read_m | PteEnum::pte_write_m);
			// map the trampoline for trap entry/exit to
			// the highest virtual address in the kernel.
			map_pages(*(PageTable*)pt_addr, TRAMPOLINE, PGSIZE, (uint64)trampoline, PteEnum::pte_read_m | PteEnum::pte_execute_m);

			Cpu* cpu = (Cpu*) hsai::get_cpu();
			// 定义适用于四级页表的 SATP 模式
			#define SATP_SV48 (9L << 60)

			// 修改 MAKE_SATP 宏以适应四级页表
			#define MAKE_SATP(pagetable) (SATP_SV48 | (((uint64)pagetable) >> 12))
			cpu->write_csr( csr::CsrAddr::satp, MAKE_SATP( pt_addr ) );

			k_tlbm.invalid_all_tlb();
		}

	} // namespace qemuk210

} // namespace riscv
