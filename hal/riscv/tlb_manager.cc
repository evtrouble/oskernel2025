//
// Created by Li shuang ( pseudonym ) on 2024-04-04 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include "tlb_manager.hh"
#include "riscv.hh"
#include "rv_cpu.hh"

#include <hsai_global.hh>
#include <virtual_cpu.hh>
#include <mem/page.hh>

namespace riscv
{
	TlbManager k_tlbm;

	void TlbManager::init( const char * lock_name )
	{
		_lock.init( lock_name );
		invalid_all_tlb();
		Cpu * cpu = ( Cpu * ) hsai::get_cpu();
		// 设置页表基地址和模式
		cpu->write_csr( csr::CsrAddr::satp, (page_table_base_address | mode) );
		// 设置地址空间标识符
		cpu->write_csr( csr::asid, 0x0UL );
		// 根据具体实现，可能需要设置其他寄存器
		// cpu->write_csr( csr::other, value );
	}
} // namespace riscv
