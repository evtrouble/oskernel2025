#include "interrupt_manager.hh"

#include <hsai_defs.h>

#include <device_manager.hh>
#include <hsai_global.hh>
#include <hsai_log.hh>

#include "include/rv_cpu.hh"
#include "include/qemu.hh"

namespace riscv
{
	namespace qemuk210
	{
		InterruptManager k_im;

		InterruptManager::InterruptManager( const char * lock_name )
		{
			_lock.init( lock_name );
			Cpu	  *cpu	 = Cpu::get_rv_cpu();

			writed(1, PLIC_V + DISK_IRQ * sizeof(uint32));
			writed(1, PLIC_V + UART_IRQ * sizeof(uint32));
			int hart = cpu->get_cpu_id();
			// set uart's enable bit for this hart's S-mode. 
			*(uint32*)PLIC_SENABLE(hart)= (1 << UART_IRQ) | (1 << DISK_IRQ);
			// set this hart's S-mode priority threshold to 0.
			*(uint32*)PLIC_SPRIORITY(hart) = 0;

			if ( register_interrupt_manager( this ) < 0 )
			{
				hsai_panic( "register interrupt manager fail." );
			}

			intr_init();
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

		int InterruptManager::handle_dev_intr()
		{
			uint32 irq = plic_claim();

			if (UART_IRQ == irq)
			{
				// uartintr();
				// hsai_warn( "uart intr not implement" );
				_uart0->handle_intr();
			}
			else if (DISK_IRQ == irq ) { _disk->handle_intr(); }
			else if (irq) {
				printf("unexpected interrupt irq = %d\n", irq);
			}
	
			if (irq) { plic_complete(irq);}

			return 1;
		}

		int InterruptManager::plic_claim(void)
		{
			int hart = Cpu::get_rv_cpu()->get_cpu_id();
			int irq;
			irq = *(uint32*)PLIC_SCLAIM(hart);
			return irq;
		}

		// tell the PLIC we've served this IRQ.
		void InterruptManager::plic_complete(int irq)
		{
			int hart = Cpu::get_rv_cpu()->get_cpu_id();
			*(uint32*)PLIC_SCLAIM(hart) = irq;
		}
	} // namespace qemuk210

} // namespace riscv
