#pragma once
#include "kernel/types.hh"
#include "uart/virtual_uart.hh"
#include "smp/spin_lock.hh"
using namespace hsai;
namespace riscv
{
	class UartConsole : public VirtualUartController
	{
	private:
		void consputc( int c );

#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x
		uint64 uart_addr;
        struct {
			hsai::SpinLock  lock;
			
			// input
			#define INPUT_BUF 128
			char buf[INPUT_BUF];
			uint r;  // Read index
			uint w;  // Write index
			uint e;  // Edit index
		} cons;
		// the transmit output buffer.
		hsai::SpinLock uart_tx_lock;
	public:

		UartConsole() = default;
		UartConsole( void *reg_base );

		virtual void init() override;
		virtual int put_char_sync( u8 c ) override;
		virtual int put_char( u8 c ) override;
		virtual int get_char_sync( u8 * c ) override;
		virtual int get_char( u8 * c ) override;
		virtual int handle_intr() override;

	public:

		virtual bool read_ready() override { return cons.r != cons.w; }
		virtual bool write_ready() override { return true; }
	};

} // namespace riscv
