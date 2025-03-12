#pragma once 

#include <smp/spin_lock.hh>

namespace riscv
{
	class TlbManager
	{
	private:
		hsai::SpinLock _lock;

	public:
		TlbManager() = default;

		void init( const char *lock_name );

		inline void invalid_all_tlb()
		{
			// the zero, zero means flush all TLB entries.
			// asm volatile("sfence.vma zero, zero");
			asm volatile("sfence.vma");
		}
	};

	extern TlbManager k_tlbm;

} // namespace riscv
