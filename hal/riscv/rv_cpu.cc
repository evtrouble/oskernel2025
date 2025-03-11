//
// Created by Li Shuang ( pseudonym ) on 2024-06-16 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include "rv_cpu.hh"
#include "context.hh"

#include <hsai_global.hh>

namespace riscv
{
	static Cpu k_rv_cpus[ NUMCPU ];
	static Context k_rv_cpu_context[ NUMCPU ];

	Cpu::Cpu() : VirtualCpu()
	{
		_context = &k_rv_cpu_context[ get_cpu_id() ];
	}

	void Cpu::_interrupt_on()
	{
		set_csr( csr::CsrAddr::sstatus, csr::sstatus_sie_m );
	}

	void Cpu::_interrupt_off()
	{
		clear_csr( csr::CsrAddr::sstatus, csr::sstatus_sie_m );
	}

	Cpu * Cpu::get_rv_cpu()
	{
		return ( Cpu * ) hsai::get_cpu();
	}

	uint Cpu::get_cpu_id()
	{
		uint64 x;
		asm volatile( "mv %0, tp" : "=r" ( x ) );
		return x;
	}

	int Cpu::is_interruptible()
	{
		uint64 x = read_csr( csr::CsrAddr::sstatus );
		return ( x & csr::sstatus_sie_m ) != 0;
	}

	uint64 Cpu::read_csr( csr::CsrAddr r )
	{
		return csr::_read_csr_( r );
	}

	void Cpu::write_csr( csr::CsrAddr r, uint64 d )
	{
		csr::_write_csr_( r, d );
	}

	void Cpu::set_csr( csr::CsrAddr r, uint64 d )
	{
		csr::_set_csr_( r, d );
	}

	void clear_csr( csr::CsrAddr r, uint64 d )
	{
		csr::_clear_csr_( r, d );
	}

} // namespace riscv

extern "C" {
	void _cpu_init()
	{
		int i;
		asm volatile( "mv %0, tp" : "=r" ( i ) );
		new ( &riscv::k_rv_cpus[i] ) riscv::Cpu;
		riscv::Cpu::register_cpu( riscv::k_rv_cpus + i, i );
		
	}
}
