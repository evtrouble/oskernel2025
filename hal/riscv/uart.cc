#include "include/uart.hh"
#include <process_interface.hh>
#include "include/console.hh"
#include <klib/printer.hh>
#include <virtual_cpu.hh>
#include "include/rv_cpu.hh"
namespace riscv
{
  UartConsole::UartConsole( void *reg_base ) { uart_addr = (uint64) reg_base; }

  void UartConsole::init()
  {
    // disable interrupts.
    WriteReg(IER, 0x00);

    // special mode to set baud rate.
    WriteReg(LCR, LCR_BAUD_LATCH);

    // LSB for baud rate of 38.4K.
    WriteReg(0, 0x03);

    // MSB for baud rate of 38.4K.
    WriteReg(1, 0x00);

    // leave set-baud mode,
    // and set word length to 8 bits, no parity.
    WriteReg(LCR, LCR_EIGHT_BITS);

    // reset and enable FIFOs.
    WriteReg(FCR, FCR_FIFO_ENABLE | FCR_FIFO_CLEAR);

    // enable transmit and receive interrupts.
    WriteReg(IER, IER_TX_ENABLE | IER_RX_ENABLE);

    uart_tx_w = uart_tx_r = 0;
    uart_tx_lock.init( "uart" );
    consoleinit();
  }


  // alternate version of uartputc() that doesn't 
  // use interrupts, for use by kernel printf() and
  // to echo characters. it spins waiting for the uart's
  // output register to be empty.
  int UartConsole::put_char_sync( u8 c )
  {
	  Cpu * cpu = Cpu::get_rv_cpu();
		cpu->push_interrupt_off();
	  if ( klib::k_printer.is_panic() ) { for ( ;; ); }
	  // wait for Transmit Holding Empty to be set in LSR.
	  while ( ( ReadReg( LSR ) & LSR_TX_IDLE ) == 0 );
	  WriteReg( THR, c );
	  cpu->pop_intterupt_off();
	  return 0;
  }

  // add a character to the output buffer and tell the
  // UART to start sending if it isn't already.
  // blocks if the output buffer is full.
  // because it may block, it can't be called
  // from interrupts; it's only suitable for use
  // by write().
  int	UartConsole::put_char( u8 c ) 
  {
    uart_tx_lock.acquire();

    if(klib::k_printer.is_panic()){
      for(;;)
        ;
    }
    while ( 1 )
    {
      if ( ( ( uart_tx_w + 1 ) % UART_TX_BUF_SIZE ) == uart_tx_r )
      {
        // buffer is full.
        // wait for uartstart() to open up space in the buffer.
        hsai::sleep_at( &uart_tx_r, uart_tx_lock );
      }
      else
      {
        uart_tx_buf[uart_tx_w] = c;
        uart_tx_w			   = ( uart_tx_w + 1 ) % UART_TX_BUF_SIZE;
        uartstart();
        uart_tx_lock.release();
        return 0;
      }
    }
  }

  int	UartConsole::get_char_sync( u8 *c )
  {
    while ( !( ReadReg( LSR ) & 0x01 ) );
    *c = ReadReg( RHR );
    return 0;
  }

  // read one input character from the UART.
  // return -1 if none is waiting.
  int	UartConsole::get_char( u8 *c )
  {
    if(ReadReg(LSR) & 0x01){
      // input data is ready.
      *c = ReadReg(RHR);
      return 0;
    }
    return -1;
  }

  // handle a uart interrupt, raised because input has
  // arrived, or the uart is ready for more output, or
  // both. called from trap.c.
  int UartConsole::handle_intr()
  {
    // read and process incoming characters.
    u8  c;
    while(get_char(&c) != -1){
      consoleintr( c );
    }

    // send buffered characters.
    uart_tx_lock.acquire();
    uartstart();
    uart_tx_lock.release();
  }

  // if the UART is idle, and a character is waiting
  // in the transmit buffer, send it.
  // caller must hold uart_tx_lock.
  // called from both the top- and bottom-half.
  void UartConsole::uartstart()
  {
    while(1){
      if(uart_tx_w == uart_tx_r){
        // transmit buffer is empty.
        return;
      }
      
      if((ReadReg(LSR) & LSR_TX_IDLE) == 0){
        // the UART transmit holding register is full,
        // so we cannot give it another byte.
        // it will interrupt when it's ready for a new byte.
        return;
      }
      
      int c = uart_tx_buf[uart_tx_r];
      uart_tx_r = (uart_tx_r + 1) % UART_TX_BUF_SIZE;
      
      // maybe uartputc() is waiting for space in the buffer.
      hsai::wakeup_at(&uart_tx_r);
      
      WriteReg(THR, c);
    }
  }
}
