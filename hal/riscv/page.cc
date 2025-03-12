//
// Created by Li Shuang ( pseudonym ) on 2024-07-12 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include <mem/page.hh>
#include <hsai_global.hh>
#include <mem/virtual_memory.hh>

#include "riscv.hh"

using namespace riscv;

namespace hsai
{
	Pte::Pte( pte_t * addr ) { _data_addr = ( pte_t* ) k_mem->to_vir( ( ulong ) addr ); }

	bool Pte::is_valid() { return *_data_addr & PteEnum::pte_valid_m; }
	bool Pte::is_dirty() { return *_data_addr & PteEnum::pte_dirty_m; }
	bool Pte::is_readable() { return *_data_addr & PteEnum::pte_read_m; }
	bool Pte::is_writable() { return *_data_addr & PteEnum::pte_write_m; }
	bool Pte::is_executable() { return *_data_addr & PteEnum::pte_execute_m; }
	bool Pte::is_present() { return *_data_addr & PteEnum::pte_valid_m; }
	bool Pte::is_dir_page() { return ( *_data_addr & (PteEnum::pte_valid_m | PteEnum::pte_read_m | PteEnum::pte_execute_m | PteEnum::pte_write_m) ) == map_dir_page_flags(); }
	bool Pte::is_super_plv() { return ( *_data_addr & PteEnum::pte_user_m ) == 0; }
	bool Pte::is_user_plv() { return ( *_data_addr & PteEnum::pte_user_m ); }

	void Pte::set_super_plv() { *_data_addr &= ~user_plv_flag(); }
	void Pte::set_user_plv() { *_data_addr |= user_plv_flag(); }

	ulong Pte::to_pa() { return *_data_addr & PteEnum::pte_ppn_m; }
	pte_t Pte::get_flags() { return *_data_addr & PteEnum::pte_flags_m; }

	pte_t Pte::map_code_page_flags() { return PteEnum::pte_valid_m | PteEnum::pte_read_m | PteEnum::pte_execute_m; }
	pte_t Pte::map_data_page_flags() { return PteEnum::pte_valid_m | PteEnum::pte_read_m | PteEnum::pte_write_m; }
	pte_t Pte::map_dir_page_flags() { return PteEnum::pte_valid_m; }
	pte_t Pte::super_plv_flag() { return 0; }
	pte_t Pte::user_plv_flag() { return PteEnum::pte_user_m; }
	pte_t Pte::valid_flag() { return PteEnum::pte_valid_m; }
} // namespace hsai
