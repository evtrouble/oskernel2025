//
// Created by Li shuang ( pseudonym ) on 2024-04-05
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use
// | Contact Author: lishuang.mk@whu.edu.cn
// --------------------------------------------------------------
//

#include "interrupt_manager.hh"

#include <hsai_defs.h>

#include <ata/ahci_driver.hh>
#include <device_manager.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>
#include <uart/uart_ns16550.hh>

#include "la_cpu.hh"
#include "qemu.hh"

namespace loongarch
{
	namespace qemu
	{
		InterruptManager k_im;

		InterruptManager::InterruptManager( const char * lock_name )
		{
			_lock.init( lock_name );

			intr_init();
			if ( register_interrupt_manager( this ) < 0 )
			{
				hsai_panic( "register interrupt manager fail." );
			}
			
			// 1. 取消中断屏蔽（启用 UART0 和 DISK 中断）
			write_itr_cfg_64b(LS7A_INT_MASK_REG, ~((0x1UL << UART0_IRQ) | (0x1UL << DISK_IRQ)));
			// 2. 配置边沿触发
			write_itr_cfg_64b(LS7A_INT_EDGE_REG, (0x1UL << UART0_IRQ) | (0x1UL << DISK_IRQ));
			// 3. 映射硬件中断号到 CPU 向量
			write_itr_cfg_8b(LS7A_INT_HTMSI_VEC_REG + UART0_IRQ, UART0_IRQ);
			write_itr_cfg_8b(LS7A_INT_HTMSI_VEC_REG + DISK_IRQ, DISK_IRQ);
			// 4. 设置中断极性（低电平有效）
			write_itr_cfg_64b(LS7A_INT_POL_REG, 0x0UL);
			// 5. 启用 EXTIOI 中断
			write_itr_cfg_64b(LOONGARCH_IOCSR_EXTIOI_EN_BASE, (0x1UL << UART0_IRQ)
					| (0x1UL << DISK_IRQ));  

			write_itr_cfg_64b(LOONGARCH_IOCSR_EXTIOI_MAP_BASE, 0x01UL);

			write_itr_cfg_64b(LOONGARCH_IOCSR_EXTIOI_ROUTE_BASE, 0x10000UL);

			// 7. 设置节点类型（HT 向量）
			write_itr_cfg_64b(LOONGARCH_IOCSR_EXRIOI_NODETYPE_BASE, 0x1);

			Cpu * cpu = Cpu::get_la_cpu();
			cpu->intr_on();
		}

		void InterruptManager::intr_init()
		{
			_uart0 =
				(hsai::UartNs16550 *) hsai::k_devm.get_char_device( DEFAULT_DEBUG_CONSOLE_NAME );
			if ( _uart0 == nullptr ) { hsai_panic( "couldn't find console device" ); }

			_disk = (DiskDriver *) hsai::k_devm.get_device( "Disk driver" );
			if ( _disk == nullptr ) { hsai_panic( "couldn't find disk device" ); }
		}

		void InterruptManager::clear_uart0_intr()
		{
			// hsai_trace(
			// 	"before clear uart0 intrn\n"
			// 	"itr status : %p\n"
			// 	"ena status : %p\n",
			// 	read_itr_cfg(
			// 		ItrCfg::itr_isr_l ),
			// 	read_itr_cfg(
			// 		ItrCfg::itr_esr_l )
			// );

			// // dev::k_console.handle_uart_intr();

			// hsai_trace(
			// 	"after clear uart0 intrn\n"
			// 	"itr status : %p\n"
			// 	"ena status : %p\n",
			// 	read_itr_cfg(
			// 		ItrCfg::itr_isr_l ),
			// 	read_itr_cfg(
			// 		ItrCfg::itr_esr_l )
			// );
		}


		int InterruptManager::handle_dev_intr()
		{
			uint64 irq = read_itr_cfg_64b(LOONGARCH_IOCSR_EXTIOI_ISR_BASE);
			hsai_printf( "interrupt irq=%d\n", irq );

			if ( irq & (1UL << UART0_IRQ) )
			{
				// uartintr();
				// hsai_warn( "uart intr not implement" );
				_uart0->handle_intr();
			}
			if ( irq & (1UL << DISK_IRQ) ) { _disk->handle_intr(); }
			write_itr_cfg_64b(LS7A_INT_CLEAR_REG, irq);
			write_itr_cfg_64b( LOONGARCH_IOCSR_EXTIOI_ISR_BASE, irq );
			// if ( irq )
			// {
			// 	hsai_error( "unexpected interrupt irq=%d\n", irq );

			// 	// apic_complete( irq );
			// 	// extioi_complete( irq );
			// }

			return 1;
		}
	} // namespace qemu

} // namespace loongarch
