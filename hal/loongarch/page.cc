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

#include "loongarch.hh"

using namespace loongarch;

namespace hsai
{
	Pte::Pte( pte_t * addr ) { _data_addr = ( pte_t* ) k_mem->to_vir( ( ulong ) addr ); }

	bool Pte::is_valid() { return *_data_addr & pte_valid_m; }
	bool Pte::is_dirty() { return *_data_addr & pte_dirty_m; }
	bool Pte::is_present() { return *_data_addr & pte_present_m; }
	bool Pte::is_writable() { return *_data_addr & pte_writable_m; }
	bool Pte::is_readable() { return !( *_data_addr & pte_no_read_m ); }
	bool Pte::is_executable() { return !( *_data_addr & pte_no_execute_m ); }
	bool Pte::is_dir_page() { return ( *_data_addr & pte_flags_m ) == map_dir_page_flags(); }
	bool Pte::is_super_plv() { return ( *_data_addr & pte_plv_m ) == plv_super; }
	bool Pte::is_user_plv() { return ( *_data_addr & pte_plv_m ) == plv_user; }

	void Pte::set_super_plv() { *_data_addr &= ~user_plv_flag(); }
	void Pte::set_user_plv() { *_data_addr |= user_plv_flag(); }

	ulong Pte::to_pa() { return  *_data_addr & pte_base_pa_m; }
	pte_t Pte::get_flags() { return *_data_addr & pte_flags_m; }

	pte_t Pte::map_code_page_flags() { return pte_present_m | ( mat_cc << pte_mat_s ); }
	pte_t Pte::map_data_page_flags() { return pte_present_m | pte_writable_m | pte_dirty_m | pte_no_execute_m | ( mat_cc << pte_mat_s ); }
	pte_t Pte::map_dir_page_flags() { return valid_flag(); }
	pte_t Pte::super_plv_flag() { return plv_super << pte_plv_s; }
	pte_t Pte::user_plv_flag() { return plv_user << pte_plv_s; }
	pte_t Pte::valid_flag() { return pte_valid_m; }

	void Pte::set_data( uint64 data ) { *_data_addr = data; }

} // namespace hsai
