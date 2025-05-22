#pragma once 

#include <klib/function.hh>
#include <smp/spin_lock.hh>

namespace riscv
{
	extern char _user_or_kernel;
	class ExceptionManager
	{
	private:
		hsai::SpinLock _lock;
		#define INTERVAL     (390000000 / 2000) // timer interrupt interval

	public:
		ExceptionManager() = default;
		void init( const char *lock_name );
		void kernel_trap();
		void user_trap();
		void user_trap_ret();
		void machine_trap();
		int dev_intr();

	public:
		void ahci_read_handle();

	// exception handler 
	private:
		void _syscall();

		ulong _get_user_data( void * proc, u64 virt_addr );

		void _print_va_page( void * proc, u64 virt_addr );

		void _print_pa_page( u64 phys_addr );

		void _print_trap_frame( void * proc );

		void set_next_timeout();
	};

	extern ExceptionManager k_em;

} // namespace riscv
