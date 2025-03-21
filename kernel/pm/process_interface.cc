//
// Created by Li Shuang ( pseudonym ) on 2024-07-11 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include <process_interface.hh>

#include "pm/process_manager.hh"
#include "mm/memlayout.hh"

namespace hsai
{
	void * get_cur_proc()
	{
		return ( void * ) pm::k_pm.get_cur_pcb();
	}

	void * get_trap_frame_from_proc( void * proc )
	{
		return ( void * ) ( ( pm::Pcb * ) proc )->get_trapframe();
	}

	ulong get_trap_frame_vir_addr()
	{
		return mm::vml::vm_trap_frame;
	}

	void proc_kill( void * proc)
	{
		return ( ( pm::Pcb * ) proc )->kill();
	}

	char * get_proc_name( void * proc)
	{
		return ( ( pm::Pcb * ) proc )->get_name();
	}

	hsai::SpinLock& get_proc_lock(void * proc) 
	{ 
		return  ( ( pm::Pcb * ) proc )->_lock; 
	}

	uint get_pid( void * proc )
	{
		return ( ( pm::Pcb * ) proc )->get_pid();
	}

	bool proc_is_killed( void * proc )
	{
		return ( ( pm::Pcb * ) proc )->is_killed();
	}

	bool proc_is_running( void * proc )
	{
		return ( ( pm::Pcb * ) proc )->get_state() == pm::ProcState::running;
	}

	void sched_proc( void * proc )
	{
		pm::k_pm.sche_proc( ( pm::Pcb * ) proc );
	}

	ulong get_kstack_from_proc( void * proc )
	{
		return ( ( pm::Pcb * ) proc )->get_kstack();
	}

	ulong get_kstack_size( void * proc )
	{
		return pm::default_proc_kstack_pages * hsai::page_size;
	}

	ulong get_pgd_addr( void * proc )
	{
		return ( ( pm::Pcb * ) proc )->get_pagetable()->get_base();
	}

	void exit_proc( void * proc, int state )
	{
		pm::k_pm.exit_proc( ( pm::Pcb * ) proc, state );
	}

	VirtualPageTable * get_pt_from_proc( void * proc )
	{
		return ( VirtualPageTable * ) ( ( pm::Pcb * ) proc )->get_pagetable();
	}

	void sleep_at( void * chan, hsai::SpinLock & lk )
	{
		pm::k_pm.sleep( chan, &lk );
	}

	void wakeup_at( void * chan )
	{
		pm::k_pm.wakeup( chan );
	}

} // namespace hsai
