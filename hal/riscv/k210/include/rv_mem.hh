#pragma once 

#include <hsai_global.hh>
#include <hsai_log.hh>
#include <mem/page.hh>

namespace riscv
{
	namespace k210
	{
		class Memory : public hsai::VirtualMemory
		{
		public:
			static void memory_init();

		public:
			virtual ulong mem_start() override;
			virtual ulong mem_size() override;

			// to virtual address 
			virtual ulong to_vir( ulong addr ) override;

			// to physical address
			virtual ulong to_phy( ulong addr ) override;

			// to the address used to access I/O
			virtual ulong to_io( ulong addr ) override;

			// to the adress for DMA to access memory
			virtual ulong to_dma( ulong addr ) override;

			// configure global page table
			virtual void config_pt( ulong pt_addr ) override;

		};


	} // namespace k210


} // namespace k210
