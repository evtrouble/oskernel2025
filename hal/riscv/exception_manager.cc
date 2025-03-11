#include "exception_manager.hh"

#include "rv_cpu.hh"
#include "riscv.hh"
#include "trap_wrapper.hh"

// #include "kernel/tm/timer_manager.hh"

// #include "kernel/pm/process.hh"
#include "trap_frame.hh"
#include "include/types.h"
#include "include/param.h"
#include "include/memlayout.h"
#include "include/riscv.h"
#include "include/spinlock.h"
#include "include/proc.h"
#include "include/sbi.h"
#include "include/plic.h"
#include "include/trap.h"
#include "include/syscall.h"
#include "include/printf.h"
#include "include/console.h"
#include "include/timer.h"
#include "include/disk.h"
// #include "kernel/pm/process_manager.hh"

// #include "kernel/mm/memlayout.hh"
// #include "kernel/mm/page_table.hh"

// #include "kernel/fs/dev/ahci_controller.hh"

// #include "syscall/syscall_handler.hh"

// #include <kernel/klib/common.hh>

#include <hsai_global.hh>
#include <hsai_log.hh>
#include <intr/virtual_interrupt_manager.hh>
#include <mem/page.hh>
#include <mem/virtual_memory.hh>
#include <mem/virtual_page_table.hh>
#include <memory_interface.hh>
#include <process_interface.hh>
#include <syscall_interface.hh>
#include <timer_interface.hh>
#include <virtual_cpu.hh>

extern "C" {
extern void kernelvec();
extern void handle_tlbr();
extern void handle_merr();
extern void uservec();
extern void userret( uint64, uint64 );
}

namespace riscv
{
	ExceptionManager k_em;
	char			 _user_or_kernel;

	void ExceptionManager::init( const char *lock_name )
	{
		_lock.init( lock_name );

		Cpu *cpu = Cpu::get_rv_cpu();
		cpu->write_csr( csr::CsrAddr::stvec, (uint64)kernelvec );
		cpu->set_csr( csr::CsrAddr::sstatus, csr::sstatus_sie_m );
		// enable supervisor-mode timer interrupts.
		cpu->set_csr(csr::CsrAddr::sie, csr::sie_seie_m | csr::sie_ssie_m | csr::sie_stie_m);
		set_next_timeout();
		hsai_printf( "ExceptionManager init\n" );
	}

	static int kernel_trap_cnt = 0;

	void ExceptionManager::kernel_trap()
	{
		Cpu *cpu = Cpu::get_rv_cpu();

		u32 sepc = cpu->read_csr( csr::CsrAddr::spec );
		u32 sstatus = cpu->read_csr( csr::CsrAddr::sstatus );
		u32 scause = cpu->read_csr( csr::CsrAddr::scause );

		if((sstatus & csr::sstatus_spp_m) == 0)
			hsai_panic("kerneltrap: not from supervisor mode");
		if ( sstatus & csr::sstatus_sie_m ) {
			hsai_panic( "kernel trap : intr on!" ); 
		}

		kernel_trap_cnt++;

		if ( kernel_trap_cnt > 4 ) hsai_panic( "kernel trap" );
		if ( cpu->is_interruptible() ) hsai_panic( "kerneltrap: interrupts enabled" );

		if((which_dev = devintr()) == 0){
			hsai_printf("scause %p\n", scause);
			hsai_printf("sepc=%p stval=%p hart=%d\n", r_sepc(), r_stval(), r_tp());
			void *proc = hsai::get_cur_proc();
			if (proc != 0) {
				hsai_printf("pid: %d, name: %s\n", hsai::get_pid(proc), hsai::get_proc_name( proc ));
			}
			hsai_panic("kerneltrap");
		}
		
		void *proc = hsai::get_cur_proc();
		// give up the CPU if this is a timer interrupt.
		if(which_dev == 2 && proc && hsai::proc_is_running(proc)) {
			hsai::sched_proc( proc );
		}

		// the yield() may have caused some traps to occur,
  		// so restore trap registers for use by kernelvec.S's sepc instruction.
		cpu->write_csr( csr::CsrAddr::sepc, sepc );
		cpu->write_csr( csr::CsrAddr::sstatus, sstatus );

		kernel_trap_cnt--;
	}

	void ExceptionManager::user_trap()
	{
		Cpu *cpu   = Cpu::get_rv_cpu();
		u64	 scause = cpu->read_csr( csr::CsrAddr::scause );

		int	   which_dev = 0;
		uint64 sstatus	 = cpu->read_csr( csr::CsrAddr::sstatus );
		if (sstatus & csr::sstatus_spp_m) 
			hsai_panic( "Usertrap: not from user mode" );

		cpu->write_csr( csr::CsrAddr::stvec, (uint64) kernelvec );

		// proc->set_user_ticks(
		// 	proc->get_user_ticks() +
		// 	( hsai::get_ticks() - proc->get_last_user_tick() )
		// );

		void	  *proc		 = hsai::get_cur_proc();
		TrapFrame *trapframe = (TrapFrame *) hsai::get_trap_frame_from_proc( proc );
		trapframe->epc		 = cpu->read_csr( csr::CsrAddr::sepc );

		if(scause == 8){
			// system call
			if ( hsai::proc_is_killed( proc ) ) 
				hsai::exit_proc( proc, -1 );

			_user_or_kernel = 'u';
			_syscall();
		} 
		else if((which_dev = dev_intr()) != 0){
			// ok
		} 
		else {
			hsai_error("usertrap(): unexpected scause %p pid=%d %s\n
				sepc=%p stval=%p\n", scause, hsai::get_pid( proc ), hsai::get_proc_name( proc )
				cpu->read_csr( csr::CsrAddr::sepc ), cpu->read_csr( csr::CsrAddr::stval ));
			// trapframedump(p->trapframe);
			hsai::proc_kill( proc );
		}

		if ( hsai::proc_is_killed( proc ) ) 
			hsai::exit_proc( proc, -1 );

		// give up the CPU if this is a timer interrupt.
		if(which_dev == 2)
			hsai::sched_proc( proc );
		
		user_trap_ret();
	}

	void ExceptionManager::user_trap_ret()
	{
		Cpu *cur_cpu = Cpu::get_rv_cpu();

		// turn off interrupts until back to user space
		cur_cpu->intr_off();

		cur_cpu->write_csr( csr::CsrAddr::stvec, (uint64) uservec );

		void	  *cur_proc	   = hsai::get_cur_proc();
		TrapFrame *trapframe   = (TrapFrame *) hsai::get_trap_frame_from_proc( cur_proc );
		trapframe->epc = cur_cpu->read_csr( csr::CsrAddr::sepc );
		trapframe->kernel_sp =
			hsai::get_kstack_from_proc( cur_proc ) + hsai::get_kstack_size( cur_proc );
		trapframe->kernel_trap	 = (uint64) &_wrp_user_trap;
		trapframe->kernel_hartid = cur_cpu->get_cpu_id();

		uint32 x  = (uint32) cur_cpu->read_csr( csr::CsrAddr::sstatus );
		x &= ~csr::sstatus_spp_m; // clear SPP to 0 for user mode
  		x |= csr::sstatus_spie_m; // enable interrupts in user mode
		cur_cpu->write_csr( csr::CsrAddr::sstatus, x );

		cur_cpu->write_csr( csr::CsrAddr::sepc, trapframe->epc );

		// cur_proc->set_last_user_tick( hsai::get_ticks() );

		volatile uint64 pgdl = hsai::get_pgd_addr( cur_proc );

		userret( hsai::get_trap_frame_vir_addr(), pgdl );
	}


	// Check if it's an external/software interrupt, 
	// and handle it. 
	// returns  2 if timer interrupt, 
	//          1 if other device, 
	//          0 if not recognized. 
	int ExceptionManager::dev_intr() 
	{
		Cpu	  *cpu	 = Cpu::get_rv_cpu();
		uint64 scause = cpu->read_csr( csr::CsrAddr::scause );

		#ifdef QEMU 
		// handle external interrupt 
		if ((0x8000000000000000L & scause) && 9 == (scause & 0xff)) 
		#else 
		// on k210, supervisor software interrupt is used 
		// in alternative to supervisor external interrupt, 
		// which is not available on k210. 
		if (0x8000000000000001L == scause && 9 == cpu->read_csr( csr::CsrAddr::stval )) 
		#endif 
		{
			rc = hsai::k_im->handle_dev_intr();
			if ( rc < 0 )
			{
				hsai_error( "im handle dev intr fail" );
				return rc;
			}
			set_next_timeout();

			#ifndef QEMU
			cpu->clear_csr( csr::CsrAddr::sip, 2 );
			cpu->set_csr( csr::CsrAddr::mie, csr::mie_msie_m | csr::mie_mtie_m | csr::mie_meie_m);
			#endif 

			return 1;
		}
		else if (0x8000000000000005L == scause) {
			rc = 0;
			if ( cpu->get_cpu_id() == 0 )
			{
				rc = hsai::handle_tick_intr();
				if ( rc < 0 )
				{
					hsai_error( "tm handle dev intr fail" );
					return rc;
				}
			}
			return 2;
		}
		return 0;
	}

	void ExceptionManager::machine_trap() { hsai_panic( "not implement" ); }

	void ExceptionManager::ahci_read_handle()
	{
		// todo:
		// 正式的中断处理函数完成后应当删除simple intr handle
		// dev::ahci::k_ahci_ctl.simple_intr_handle();

		// dev::ahci::k_ahci_ctl.intr_handle();
		hsai_panic( "ahci intr" );
	}

	void ExceptionManager::set_next_timeout()
	{
		Cpu *cpu = Cpu::get_rv_cpu();
		cpu->write_csr( csr::CsrAddr::timecmp, cpu->read_csr(csr::CsrAddr::time) + INTERVAL );
	}

	static inline uint64 read_mtime()
	{
		uint64 mtime;
		asm volatile ("csrr %0, time" : "=r"(mtime));
		return mtime;
	}

	static inline void write_mtimecmp(uint64 time)
	{
		asm volatile ("csrw timecmp, %0" : : "r"(time));
	}

	// ---------------- private helper functions ----------------

	void ExceptionManager::_syscall()
	{
		void	  *p  = hsai::get_cur_proc();
		TrapFrame *tf = (TrapFrame *) hsai::get_trap_frame_from_proc( p );

		if ( hsai::proc_is_killed( p ) ) hsai::exit_proc( p, -1 );

		// update pc
		// sepc points to the ecall instruction,
		// but we want to return to the next instruction.
		tf->epc += 4;

		// tmm::k_tm.close_ti_intr();
		// an interrupt will change sstatus &c registers,
		// so don't enable until done with those registers.
		Cpu *cpu = Cpu::get_rv_cpu();
		cpu->intr_on();

		uint64 num;

		// printf( "sys a0=0x%x a7=%d\n", p->get_trapframe()->a0,
		// p->get_trapframe()->a7 );
		num = tf->a7;
		if ( num > 0 && num < hsai::get_syscall_max_num() )
		{
			// printf( "syscall num: %d\n", num );
			tf->a0 = hsai::kernel_syscall( num );
		}
		else
		{
			hsai_printf( "%d : unknown sys call %d\n", hsai::get_pid( p ), num );
			tf->a0 = -1;
		}

		// tmm::k_tm.open_ti_intr();
	}

	ulong ExceptionManager::_get_user_data( void *proc, u64 virt_addr )
	{
		hsai::VirtualPageTable *pt = hsai::get_pt_from_proc( proc );
		u64						pa = pt->walk_addr( virt_addr );
		pa						   = hsai::k_mem->to_vir( pa );
		return *( (ulong *) pa );
	}

	void ExceptionManager::_print_va_page( void *proc, u64 virt_addr )
	{
		hsai::VirtualPageTable *pt = hsai::get_pt_from_proc( proc );
		u64						pa = pt->walk_addr( virt_addr );
		pa						   = hsai::page_round_down( pa );
		pa						   = hsai::k_mem->to_vir( pa );

		u8 *p		 = (u8 *) pa;
		u64 va_start = hsai::page_round_down( virt_addr );

		hsai_printf( BLUE_COLOR_PRINT "print the page data containing va(%p)\n" CLEAR_COLOR_PRINT,
					 virt_addr );
		hsai_printf( "%p(ph)  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n", pa );
		for ( uint i = 0; i < hsai::page_size; ++i )
		{
			if ( i % 0x10 == 0 ) hsai_printf( "%p      ", va_start + i );
			hsai_printf( "%B ", p[i] );
			if ( i % 0x10 == 0xF ) hsai_printf( "\n" );
		}
	}

	void ExceptionManager::_print_pa_page( u64 phys_addr )
	{
		phys_addr = hsai::k_mem->to_vir( phys_addr );
		u64 pa	  = hsai::page_round_down( phys_addr );

		u8 *p = (u8 *) pa;

		hsai_printf( BLUE_COLOR_PRINT "print the page data containing pa(%p)\n" CLEAR_COLOR_PRINT,
					 phys_addr );
		hsai_printf( "%p(ph)  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n", pa );
		for ( uint i = 0; i < hsai::page_size; ++i )
		{
			if ( i % 0x10 == 0 ) hsai_printf( "%p      ", pa + i );
			hsai_printf( "%B ", p[i] );
			if ( i % 0x10 == 0xF ) hsai_printf( "\n" );
		}
	}

	void ExceptionManager::_print_trap_frame( void *proc )
	{
		TrapFrame *tf = (TrapFrame *) hsai::get_trap_frame_from_proc( proc );
		hsai_printf( BLUE_COLOR_PRINT "print trap frame:\n" );
		hsai_printf( "\tra(r1)  = %p\n", tf->ra );
		hsai_printf( "\ttp(r2)  = %p\n", tf->tp );
		hsai_printf( "\tsp(r3)  = %p\n", tf->sp );
		hsai_printf( "\ta0(r4)  = %p\n", tf->a0 );
		hsai_printf( "\ta1(r5)  = %p\n", tf->a1 );
		hsai_printf( "\ta2(r6)  = %p\n", tf->a2 );
		hsai_printf( "\ta3(r7)  = %p\n", tf->a3 );
		hsai_printf( "\ta4(r8)  = %p\n", tf->a4 );
		hsai_printf( "\ta5(r9)  = %p\n", tf->a5 );
		hsai_printf( "\ta6(r10) = %p\n", tf->a6 );
		hsai_printf( "\ta7(r11) = %p\n", tf->a7 );
		hsai_printf( "\tt0(r12) = %p\n", tf->t0 );
		hsai_printf( "\tt1(r13) = %p\n", tf->t1 );
		hsai_printf( "\tt2(r14) = %p\n", tf->t2 );
		hsai_printf( "\tt3(r15) = %p\n", tf->t3 );
		hsai_printf( "\tt4(r16) = %p\n", tf->t4 );
		hsai_printf( "\tt5(r17) = %p\n", tf->t5 );
		hsai_printf( "\tt6(r18) = %p\n", tf->t6 );
		hsai_printf( "\tt7(r19) = %p\n", tf->t7 );
		hsai_printf( "\tt8(r20) = %p\n", tf->t8 );
		hsai_printf( "\tr21     = %p\n", tf->r21 );
		hsai_printf( "\tfp(r22) = %p\n", tf->fp );
		hsai_printf( "\ts0(r23) = %p\n", tf->s0 );
		hsai_printf( "\ts1(r24) = %p\n", tf->s1 );
		hsai_printf( "\ts2(r25) = %p\n", tf->s2 );
		hsai_printf( "\ts3(r26) = %p\n", tf->s3 );
		hsai_printf( "\ts4(r27) = %p\n", tf->s4 );
		hsai_printf( "\ts5(r28) = %p\n", tf->s5 );
		hsai_printf( "\ts6(r29) = %p\n", tf->s6 );
		hsai_printf( "\ts7(r30) = %p\n", tf->s7 );
		hsai_printf( "\ts8(r31) = %p\n", tf->s8 );
		hsai_printf( CLEAR_COLOR_PRINT );
	}

} // namespace loongarch

