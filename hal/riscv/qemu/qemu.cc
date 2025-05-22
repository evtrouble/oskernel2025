#include "include/qemu.hh"

#include <device_manager.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <intr/virtual_interrupt_manager.hh>
#include <memory_interface.hh>
#include <process_interface.hh>
#include <timer_interface.hh>
#include <mm/virtual_memory_manager.hh>

#include "context.hh"
#include "exception_manager.hh"
#include "include/interrupt_manager.hh"
#include "rv_cpu.hh"
#include "rv_mem.hh"
#include "trap_frame.hh"
#include "uart.hh"

namespace riscv
{
	namespace qemu
	{
		UartConsole debug_uart;

		DiskDriver disk_driver;
	} // namespace qemu

} // namespace riscv

extern "C" {
extern uint64 _start_u_init;
extern uint64 _end_u_init;
extern uint64 _u_init_stks;
extern uint64 _u_init_stke;
extern uint64 _u_init_txts;
extern uint64 _u_init_txte;
extern uint64 _u_init_dats;
extern uint64 _u_init_date;
extern int	  init_main( void );
extern char trampoline[];
extern char	  __global_pointer$[];
}

extern "C" {
extern ulong _bss_start_addr;
extern ulong _bss_end_addr;
extern ulong kernel_end;
}
using namespace riscv;
using namespace riscv::qemu;

namespace hsai
{
	const u64 memory_start = kernel_end;
	const u64 memory_size  = PHYSTOP - kernel_end;

	const uint context_size = sizeof( riscv::Context );

	constexpr uint64 _1K_dec = 1000UL;
	constexpr uint64 _1M_dec = _1K_dec * _1K_dec;
	constexpr uint64 _1G_dec = _1M_dec * _1K_dec;

	// 以下两个常量表达式应当是绑定在一起的

	// 参考测例说明的时钟频率是12.5MHz,
	// 但是测试发现比这个值要小一些，大约是四分之一 constexpr uint64 qemu_fre =
	// 12 * _1M_dec + 500 * _1K_dec; constexpr uint64 qemu_fre = 100 * _1M_dec;
	// // 按照 CPUCFG.4 的值则恰为 100'000'000
	constexpr uint64 qemu_fre = 3 * _1M_dec + 125 * _1K_dec;
	// 由cycles计算出微秒的方法
	constexpr uint64 qemu_fre_cal_usec( uint64 cycles ) { return cycles * 8 / 25; }
	// 由微秒计算出cycles的方法
	constexpr uint64 qemu_fre_cal_cycles( uint64 usec ) { return usec * 25 / 8; }

	// 100K 分频，则时间片 100K/3.125MHz ~ 32ms
	// constexpr uint div_fre = 0x80000000UL;
	constexpr uint div_fre	   = ( 200 * _1K_dec ) >> 2; // 低两位由硬件补齐
	constexpr uint ms_per_tick = div_fre * _1K_dec / qemu_fre;

	void hardware_abstract_init( void )
	{
		// 1. 使用 UART0 作为 debug 输出
		new ( &qemu::debug_uart ) UartConsole(
			(void*) ( UART) );
		qemu::debug_uart.init();
		register_debug_uart( &qemu::debug_uart );

		// 2. 架构初始化
		riscv_init();

		// 3. 初始化 Memory
		qemu::Memory::memory_init();
	}

	void identify_device()
	{
		// Disk 识别设备（需要在中断初始化后进行）
		DiskDriver* disk = (DiskDriver*) k_devm.get_device( "Disk driver" );
		disk->identify_device();
	}

	void hardware_secondary_init()
	{
		// 关闭非对齐访存检查
		// Cpu*  rvcpu = (Cpu*) hsai::get_cpu();
		// rvcpu->set_csr( csr::CsrAddr::sstatus, csr::sstatus_aie_m );

		// 1. 异常管理初始化
		riscv::k_em.init( "exception manager" );

		// 2. Disk 初始化 (debug)
		new ( &disk_driver ) DiskDriver( "Disk");

		// 3. 中断管理初始化
		new ( &riscv::qemu::k_im )
			riscv::qemu::InterruptManager( "intr manager" );
		hsai_info( "im init" );
	}

	void user_proc_init( void* proc )
	{
		TrapFrame* tf = (TrapFrame*) get_trap_frame_from_proc( proc );
		tf->epc		  = (uint64) &init_main - (uint64) &_start_u_init;
		hsai_info( "user init: epc = %p", tf->epc );
		mm::PageTable *pt = (mm::PageTable*) get_pt_from_proc( proc );
		mm::k_vmm.map_code_pages(*pt, TRAMPOLINE, 
			PG_SIZE, (uint64) trampoline, false );
		tf->gp = (uint64)__global_pointer$;
		
		// tf->sp = ( uint64 ) &_u_init_stke - ( uint64 ) &_start_u_init;
		hsai_info( "user init: gp  = %p", tf->gp );
	}

	void proc_init( void * proc )
	{
		mm::PageTable& pt = *(mm::PageTable*) get_pt_from_proc( proc );
		mm::k_vmm.map_code_pages(pt, TRAMPOLINE, 
			PG_SIZE, (uint64) trampoline, false );
	}

	void proc_free( void * proc )
	{
		mm::PageTable& pt = *(mm::PageTable*) get_pt_from_proc( proc );
		mm::k_vmm.vm_unmap( pt, TRAMPOLINE, 1, 0 );
	}

	void set_trap_frame_return_value( void* trapframe, ulong v )
	{
		TrapFrame* tf = (TrapFrame*) trapframe;
		tf->a0		  = v;
	}

	void set_trap_frame_entry( void* trapframe, void* entry )
	{
		TrapFrame* tf = (TrapFrame*) trapframe;
		tf->epc		  = (u64) entry;
	}

	void set_trap_frame_user_sp( void* trapframe, ulong sp )
	{
		TrapFrame* tf = (TrapFrame*) trapframe;
		tf->sp		  = sp;
	}

	void set_trap_frame_arg( void* trapframe, uint arg_num, ulong value )
	{
		TrapFrame* tf = (TrapFrame*) trapframe;
		switch ( arg_num )
		{
			case 0 : tf->a0 = value; break;
			case 1 : tf->a1 = value; break;
			case 2 : tf->a2 = value; break;
			case 3 : tf->a3 = value; break;
			case 4 : tf->a4 = value; break;
			case 5 : tf->a5 = value; break;
			case 6 : tf->a6 = value; break;
			case 7 : tf->a7 = value; break;
			default: hsai_panic( "invalid argument number" ); break;
		}
	}

	void copy_trap_frame( void* from, void* to )
	{
		TrapFrame* tf_f = (TrapFrame*) from;
		TrapFrame* tf_t = (TrapFrame*) to;
		*tf_t			= *tf_f;
	}

	ulong get_arg_from_trap_frame( void* trapframe, uint arg_num )
	{
		TrapFrame* tf = (TrapFrame*) trapframe;
		switch ( arg_num )
		{
			case 0: return tf->a0;
			case 1: return tf->a1;
			case 2: return tf->a2;
			case 3: return tf->a3;
			case 4: return tf->a4;
			case 5: return tf->a5;
			default:
			{
				hsai_panic( "invalid argument number" );
				return 0;
			}
		}
		return -1;
	}

	void user_trap_return() { k_em.user_trap_ret(); }

	void set_context_entry( void* cont, void* entry )
	{
		Context* ct = (Context*) cont;
		ct->ra		= (ulong) entry;
	}

	void set_context_sp( void* cont, ulong sp )
	{
		Context* ct = (Context*) cont;
		ct->sp		= sp;
	}

	void* get_context_address( uint proc_gid )
	{
		if ( proc_gid >= proc_pool_size ) return nullptr;
		return &proc_context_pool()[proc_gid];
	}

	ulong get_main_frequence() { return qemu_fre; }

	ulong get_hw_time_stamp() { return ( (Cpu*) hsai::get_cpu() )->get_time(); }

	ulong time_stamp_to_usec( ulong ts ) { return qemu_fre_cal_usec( ts ); }

	ulong usec_to_time_stamp( ulong us ) { return qemu_fre_cal_cycles( us ); }

	ulong cycles_per_tick() { return div_fre << 2; }

} // namespace hsai
