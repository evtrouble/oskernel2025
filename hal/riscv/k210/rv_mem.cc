//
// Created by Li Shuang ( pseudonym ) on 2024-07-10
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "include/rv_mem.hh"

#include <hsai_global.hh>
#include <hsai_log.hh>
#include <mem/page.hh>
#include <memory_interface.hh>
#include <mm/virtual_memory_manager.hh>

#include "rv_cpu.hh"
#include "riscv.hh"
#include "include/k210.hh"
#include "tlb_manager.hh"

extern "C" {
	extern char kernel_end[]; // first address after kernel.
	extern char	 etext[];
	extern char trampoline[];
}

namespace riscv
{
	namespace k210
	{
		static Memory k_k210_mem;

		void Memory::memory_init()
		{
			register_memory( &k_k210_mem );
		}

		ulong Memory::mem_start()
		{
			ulong end_addr	= (ulong) &kernel_end;
			end_addr	   += _1M - 1;
			end_addr	   &= ~( _1M - 1 );
			return end_addr;
		}
		ulong Memory::mem_size()
		{
			return PHYSTOP - mem_start();
		}

		ulong Memory::to_vir( ulong addr ) { return addr; }

		ulong Memory::to_phy( ulong addr ) { return addr; }

		ulong Memory::to_io( ulong addr ) { return addr; }

		ulong Memory::to_dma( ulong addr ) { return addr; }

		void Memory::config_pt( ulong pt_addr )
		{
			// uart registers
			mm::k_vmm.map_data_pages(mm::k_pagetable, UART, PG_SIZE, UART, false);
			
			// CLINT
			mm::k_vmm.map_data_pages(mm::k_pagetable, CLINT_V, 0x10000, CLINT, false);

			// PLIC
			mm::k_vmm.map_data_pages(mm::k_pagetable, PLIC_V, 0x4000, PLIC, false);
			mm::k_vmm.map_data_pages(mm::k_pagetable, PLIC_V + 0x200000, 0x4000, PLIC + 0x200000, false);

			// GPIOHS
			mm::k_vmm.map_data_pages(mm::k_pagetable, GPIOHS_V, 0x1000, GPIOHS, false);

			// DMAC
			mm::k_vmm.map_data_pages(mm::k_pagetable, DMAC_V, 0x1000, DMAC, false);

			// GPIO
			// map_pages(*(PageTable*)pt_addr, GPIO_V, 0x1000, GPIO, false);

			// SPI_SLAVE
			mm::k_vmm.map_data_pages(mm::k_pagetable, SPI_SLAVE_V, 0x1000, SPI_SLAVE, false);

			// FPIOA
			mm::k_vmm.map_data_pages(mm::k_pagetable, FPIOA_V, 0x1000, FPIOA, false);

			// SPI0
			mm::k_vmm.map_data_pages(mm::k_pagetable, SPI0_V, 0x1000, SPI0, false);

			// SPI1
			mm::k_vmm.map_data_pages(mm::k_pagetable, SPI1_V, 0x1000, SPI1, false);

			// SPI2
			mm::k_vmm.map_data_pages(mm::k_pagetable, SPI2_V, 0x1000, SPI2, false);

			// SYSCTL
			mm::k_vmm.map_data_pages(mm::k_pagetable, SYSCTL_V, 0x1000, SYSCTL, false);
			
			// map rustsbi
			// kvmmap(RUSTSBI_BASE, RUSTSBI_BASE, KERNBASE - RUSTSBI_BASE, PTE_R | PTE_X);
			// map kernel text executable and read-only.
			mm::k_vmm.map_code_pages(mm::k_pagetable, KERNBASE, (uint64)etext - KERNBASE, KERNBASE, false);
			// map kernel data and the physical RAM we'll make use of.
			mm::k_vmm.map_data_pages(mm::k_pagetable, (uint64)etext, PHYSTOP - (uint64)etext, (uint64)etext, false);
			// map the trampoline for trap entry/exit to
			// the highest virtual address in the kernel.
			mm::k_vmm.map_code_pages(mm::k_pagetable, TRAMPOLINE, PG_SIZE, (uint64)trampoline, false);

			Cpu* cpu = (Cpu*) hsai::get_cpu();
			cpu->write_csr( csr::CsrAddr::satp, MAKE_SATP( pt_addr ) );

			k_tlbm.invalid_all_tlb();
		}

	} // namespace k210
	
} // namespace riscv
