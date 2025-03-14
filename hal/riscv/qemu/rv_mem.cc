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
#include <memory_interface.hh>
#include <kernel/mm/virtual_memory_manager.hh>

#include "rv_cpu.hh"
#include "riscv.hh"
#include "include/qemu.hh"
#include "include/rv_mem.hh"
#include "tlb_manager.hh"

extern "C" {
extern ulong kernel_end;
extern ulong etext;
}

namespace riscv
{
	namespace qemu
	{
		static Memory k_qemu_mem;

		void Memory::memory_init()
		{
			register_memory( &k_qemu_mem );
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
			hsai::PageTable* addr = (hsai::PageTable*) pt_addr;
			// uart registers
			k_vmm.map_pages(*addr, UART_V, PG_SIZE, UART, PteEnum::pte_read_m | PteEnum::pte_write_m);
			
			// virtio mmio disk interface
			k_vmm.map_pages(*addr, VIRTIO0_V, PG_SIZE, VIRTIO0, pte_read_m | pte_write_m);
			// CLINT
			k_vmm.map_pages(*addr, CLINT_V, 0x10000, CLINT, PteEnum::pte_read_m | PteEnum::pte_write_m);

			// PLIC
			k_vmm.map_pages(*addr, PLIC_V, 0x4000, PLIC, PteEnum::pte_read_m | PteEnum::pte_write_m);
			k_vmm.map_pages(*addr, PLIC_V + 0x4000, PG_SIZE, PLIC + 0x200000, PteEnum::pte_read_m | PteEnum::pte_write_m);
			
			// map rustsbi
			// kvmmap(RUSTSBI_BASE, RUSTSBI_BASE, KERNBASE - RUSTSBI_BASE, PTE_R | PTE_X);
			// map kernel text executable and read-only.
			k_vmm.map_pages(*addr, KERNBASE, (uint64)etext - KERNBASE, KERNBASE, PteEnum::pte_read_m | PteEnum::pte_execute_m);
			// map kernel data and the physical RAM we'll make use of.
			k_vmm.map_pages(*addr, (uint64)etext, PHYSTOP - (uint64)etext, (uint64)etext, PteEnum::pte_read_m | PteEnum::pte_write_m);
			// map the trampoline for trap entry/exit to
			// the highest virtual address in the kernel.
			k_vmm.map_pages(*addr, TRAMPOLINE, PG_SIZE, (uint64)trampoline, PteEnum::pte_read_m | PteEnum::pte_execute_m);

			Cpu* cpu = (Cpu*) hsai::get_cpu();
			// 定义适用于四级页表的 SATP 模式
			#define SATP_SV48 (9L << 60)

			// 修改 MAKE_SATP 宏以适应四级页表
			#define MAKE_SATP(pagetable) (SATP_SV48 | (((uint64)pagetable) >> 12))
			cpu->write_csr( csr::CsrAddr::satp, MAKE_SATP( pt_addr ) );

			k_tlbm.invalid_all_tlb();
		}

	} // namespace qemu

} // namespace riscv
