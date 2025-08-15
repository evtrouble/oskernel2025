    .section .text.osstartup
    .globl _entry
_entry:
    # set up a stack for C.
    # entry_stack is declared in start.c,
    # with a 4096-byte stack per CPU.
    # sp = entry_stack + (hartid * 4096)

    # Load entry_stack address
    mv tp, a0
    la          sp, entry_stack

    # Load stack size
    li          a0, 0x4000

    mv          a1, tp      
    addi        a1, a1, 1         

    # Calculate stack offset: (hartid) * stack_size
    mul         a0, a0, a1                 

    # Set stack pointer
    add         sp, sp, a0
    call _cpu_init

	li fp, 0
    la gp, __global_pointer$
    call xn6_start_kernel

loop:
    j loop
