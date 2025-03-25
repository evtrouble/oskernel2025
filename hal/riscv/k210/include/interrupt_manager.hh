#pragma once 

#include <smp/spin_lock.hh>
#include <intr/virtual_interrupt_manager.hh>
#include "disk_driver.hh"
#include "uart.hh"

namespace riscv
{
	namespace k210
	{
		class InterruptManager : public hsai::VirtualInterruptManager
		{
		private:
			hsai::SpinLock _lock;
			UartConsole * _uart0 = nullptr;
			DiskDriver * _sata = nullptr;

			int plic_claim( void );
			void plic_complete( int irq );
			#define readb(addr) (*(volatile uint8 *)(addr))
			#define readw(addr) (*(volatile uint16 *)(addr))
			#define readd(addr) (*(volatile uint32 *)(addr))
			#define readq(addr) (*(volatile uint64 *)(addr))

			#define writeb(v, addr)                      \
				{                                        \
					(*(volatile uint8 *)(addr)) = (v); \
				}
			#define writew(v, addr)                       \
				{                                         \
					(*(volatile uint16 *)(addr)) = (v); \
				}
			#define writed(v, addr)                       \
				{                                         \
					(*(volatile uint32 *)(addr)) = (v); \
				}
			#define writeq(v, addr)                       \
				{                                         \
					(*(volatile uint64 *)(addr)) = (v); \
				}

			#define UART_IRQ    33
			#define DISK_IRQ    27

		public:
			InterruptManager() = default;
			InterruptManager( const char *lock_name );
			void intr_init();

			virtual int handle_dev_intr() override;
		};

		extern InterruptManager k_im;

	} // namespace k210

} // namespace riscv
