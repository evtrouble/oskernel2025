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
	}
} // namespace riscv
