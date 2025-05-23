//
// Created by Li Shuang ( pseudonym ) on 2024-06-15 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#pragma once

#include "kernel/types.hh"
#include "kernel/mm/page_table.hh"

namespace pm
{
	class Pcb;
} // namespace pm


namespace hsai	// hardware service abstract interface
{
	class VirtualCpu __hsai_hal
	{
	protected:
		int _num_off = 0;		// number of interrupt's off
		int _int_ena = 0;		// interrupt enable status before push-off()
		pm::Pcb * _cur_proc = nullptr;
		void * _context = nullptr;

	public:
		VirtualCpu() = default;

		static int register_cpu( VirtualCpu * p_cpu, int cpu_id );

		virtual uint get_cpu_id() = 0;
		virtual int is_interruptible() = 0;
		virtual void _interrupt_on() = 0;
		virtual void _interrupt_off() = 0;
		virtual void set_mmu(mm::PageTable& pt) = 0;

	public:
		int get_num_off() { return _num_off; }
		int get_int_ena() { return _int_ena; }
		pm::Pcb * get_cur_proc() { return _cur_proc; }
		void * get_context() { return _context; }

		void set_cur_proc( pm::Pcb * proc ) { _cur_proc = proc; }
		void set_int_ena( int int_ena ) { _int_ena = int_ena; }

	public:
		void push_interrupt_off();
		void pop_intterupt_off();
	};

	// constexpr int max_cpu_num = 4;

} // namespace hsai
