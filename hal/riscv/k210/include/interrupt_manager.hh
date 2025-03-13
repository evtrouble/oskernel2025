#pragma once 

#include <smp/spin_lock.hh>
#include <intr/virtual_interrupt_manager.hh>

namespace hsai
{
	class UartNs16550;
	class AhciDriver;
} // namespace hsai

namespace riscv
{
	namespace k210
	{
		class InterruptManager : public hsai::VirtualInterruptManager
		{
		private:
			hsai::SpinLock _lock;
			hsai::UartNs16550 * _uart0 = nullptr;
			hsai::AhciDriver * _sata = nullptr;

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

		public:
			InterruptManager() = default;
			InterruptManager( const char *lock_name );
			void intr_init();

			virtual int handle_dev_intr() override;
		};

		extern InterruptManager k_im;

	} // namespace k210

} // namespace riscv
