#pragma once 

#include "riscv.hh"

#include <kernel/types.hh>
#include <virtual_cpu.hh>

extern "C" {
	void _cpu_init();
}

namespace riscv
{
	class Cpu : public hsai::VirtualCpu
	{
	protected:
		virtual void _interrupt_on() override;
		virtual void _interrupt_off() override;

	public:
		Cpu();
		static Cpu * get_rv_cpu();

		virtual uint get_cpu_id() override;
		virtual int is_interruptible() override;

		uint64 read_csr( csr::CsrAddr r );
		void write_csr( csr::CsrAddr r, uint64 d );
		void set_csr( csr::CsrAddr r, uint64 d );
		void clear_csr( csr::CsrAddr r, uint64 d );
		uint64 get_time();

		void intr_on() { _interrupt_on(); }
		void intr_off() { _interrupt_off(); }
	};

} // namespace riscv
