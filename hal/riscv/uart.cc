#include "include/uart.hh"
#include <process_interface.hh>
#include <klib/printer.hh>
#include <virtual_cpu.hh>
#include "include/rv_cpu.hh"
#include "include/sbi.hh"
namespace riscv
{
  //
  // Console input and output, to the uart.
  // Reads are line at a time.
  // Implements special input characters:
  //   newline -- end of line
  //   control-h -- backspace
  //   control-u -- kill line
  //   control-d -- end of file
  //   control-p -- print process list
  //

  UartConsole::UartConsole( void *reg_base ) { uart_addr = (uint64) reg_base; }

  void UartConsole::init()
  {
    cons.lock.init( "cons" );

    cons.e = cons.w = cons.r = 0;
  }

  void UartConsole::consputc(int c) {
    if(c == BACKSPACE){
      // if the user typed backspace, overwrite with a space.
      sbi_console_putchar('\b');
      sbi_console_putchar(' ');
      sbi_console_putchar('\b');
    } else {
      sbi_console_putchar(c);
    }
  }

  // alternate version of uartputc() that doesn't 
  // use interrupts, for use by kernel printf() and
  // to echo characters. it spins waiting for the uart's
  // output register to be empty.
  int UartConsole::put_char_sync( u8 c )
  {
	  consputc( c );
	  return 0;
  }

  // add a character to the output buffer and tell the
  // UART to start sending if it isn't already.
  // blocks if the output buffer is full.
  // because it may block, it can't be called
  // from interrupts; it's only suitable for use
  // by write().
  int UartConsole::put_char( u8 c ) { consputc( c ); }

  int	UartConsole::get_char_sync( u8 *c )
  {
    cons.lock.acquire();
    while(cons.r == cons.w){
      if(hsai::proc_is_killed( hsai::get_cur_proc() ) ){
        cons.lock.release();
        return -1;
      }
      hsai::sleep_at(&cons.r, cons.lock);
    }
  
    *c = cons.buf[cons.r++ % INPUT_BUF];
    cons.lock.release();

    return 0;
  }

  // read one input character from the UART.
  // return -1 if none is waiting.
  int	UartConsole::get_char( u8 *c )
  {
    cons.lock.acquire();
    if(cons.r == cons.w)
      return -1;
  
    *c = cons.buf[cons.r++ % INPUT_BUF];
    cons.lock.release();

    return 0;
  }

  //
  // the console input interrupt handler.
  // uartintr() calls this for input character.
  // do erase/kill processing, append to cons.buf,
  // wake up consoleread() if a whole line has arrived.
  //
  int UartConsole::handle_intr()
  {
    int c = sbi_console_getchar();
    cons.lock.acquire();

    switch ( c )
    {
      // case C( 'P' ): // Print process list.
      // 	procdump();
      // 	break;
      case C( 'U' ): // Kill line.
        while ( cons.e != cons.w && cons.buf[( cons.e - 1 ) % INPUT_BUF] != '\n' )
        {
          cons.e--;
          consputc( BACKSPACE );
        }
        break;
      case C( 'H' ): // Backspace
      case '\x7f':
        if ( cons.e != cons.w )
        {
          cons.e--;
          consputc( BACKSPACE );
        }
        break;
      default:
        if ( c != 0 && cons.e - cons.r < INPUT_BUF )
        {
        #ifndef QEMU
        if (c == '\r') break;     // on k210, "enter" will input \n and \r
        #else
        c = (c == '\r') ? '\n' : c;
        #endif
        // echo back to the user.
        consputc(c);

        // store for consumption by consoleread().
        cons.buf[cons.e++ % INPUT_BUF] = c;

        if(c == '\n' || c == C('D') || cons.e == cons.r+INPUT_BUF){
          // wake up consoleread() if a whole line (or end-of-file)
          // has arrived.
          cons.w = cons.e;
          hsai::wakeup_at(&cons.r);
        }
      }
      break;
    }
    cons.lock.release();
  }
}
