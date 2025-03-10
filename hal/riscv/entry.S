#include "asm.h"

.section .text.osstartup
.global _entry
.global __init_atexit_func_entry
.global _cpu_init
_entry:
        la.global   t0, 0f
        j           t0
0:      li          t0, 0x9000000000000011   # CA, PLV0, 0x9000 xxxx xxxx xxxx
        csrw        RISCV_CSR_PMPADDR0, t0 
        li          t0, 0x8000000000000001   # UA, PLV0, 0x8000 xxxx xxxx xxxx   
        csrw        RISCV_CSR_PMPADDR0, t0
        li          t0, 0x0000000000000000 
        csrw        RISCV_CSR_PMPADDR0, t0
        csrw        RISCV_CSR_PMPADDR0, t0
        csrw        RISCV_CSR_STVEC, t0	
        li          t0, 0xb0              # PLV=0, IE=0, PG=1
        csrw        RISCV_CSR_MSTATUS, t0
        sfence.vma  zero, zero

	# set up a stack for C.
        # entry_stack is declared in start.c,
        # with a 4096-byte stack per CPU.
        # sp = entry_stack + (hartid * 4096)
     
        la          sp, entry_stack
        li          a0, RISCV_ENTRY_STACK_SIZE
        csrr        a1, RISCV_CSR_MHARTID
        addi        tp, a1, 0
        addi        a1, a1, 1
        mul         a0, a0, a1
        add         sp, sp, a0

        bnez        tp, _skip_clear_bss
        la          t0, _bss_start_addr
        la          t1, _bss_end_addr
        li          t2, 0
_loop_clear_bss:
        bgeu        t0, t1, _skip_clear_bss
        sd          t2, 0(t0)
        addi        t0, t0, 8
        j           _loop_clear_bss
_skip_clear_bss:

_test_init_atexit:
        # call        __init_atexit_func_entry

        call        _cpu_init

        # add         t0, zero, t0
        # call        -4

        li          fp, 0

	# jump to xn6_start_kernel()
        call        xn6_start_kernel
spin:
        j           spin
