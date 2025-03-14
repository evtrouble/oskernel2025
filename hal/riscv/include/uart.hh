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
		uint64 uart_addr;
        // the UART control registers are memory-mapped
		// at address UART0. this macro returns the
		// address of one of the registers.
		#define Reg(reg) ((volatile unsigned char *)(uart_addr + reg))

		// the UART control registers.
		// some have different meanings for
		// read vs write.
		// see http://byterunner.com/16550.html
		#define RHR 0                 // receive holding register (for input bytes)
		#define THR 0                 // transmit holding register (for output bytes)
		#define IER 1                 // interrupt enable register
		#define IER_TX_ENABLE (1<<0)
		#define IER_RX_ENABLE (1<<1)
		#define FCR 2                 // FIFO control register
		#define FCR_FIFO_ENABLE (1<<0)
		#define FCR_FIFO_CLEAR (3<<1) // clear the content of the two FIFOs
		#define ISR 2                 // interrupt status register
		#define LCR 3                 // line control register
		#define LCR_EIGHT_BITS (3<<0)
		#define LCR_BAUD_LATCH (1<<7) // special mode to set baud rate
		#define LSR 5                 // line status register
		#define LSR_RX_READY (1<<0)   // input is waiting to be read from RHR
		#define LSR_TX_IDLE (1<<5)    // THR can accept another character to send

		#define ReadReg(reg) (*(Reg(reg)))
		#define WriteReg(reg, v) (*(Reg(reg)) = (v))

		// the transmit output buffer.
		hsai::SpinLock uart_tx_lock;
		#define UART_TX_BUF_SIZE 32
		char uart_tx_buf[UART_TX_BUF_SIZE];
		int uart_tx_w; // write next to uart_tx_buf[uart_tx_w++]
		int uart_tx_r; // read next from uart_tx_buf[uar_tx_r++]

		void uartstart();

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
		virtual bool read_ready() override { return ReadReg( LSR ) & 0x01; }
		virtual bool write_ready() override { return (ReadReg(LSR) & LSR_TX_IDLE); }
	};

} // namespace riscv
