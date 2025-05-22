    .section .text.osstartup
    .globl _entry
_entry:
    la sp, entry_stack
    mv tp, a0
    call _cpu_init

	li fp, 0
    la gp, __global_pointer$
    call xn6_start_kernel

loop:
    j loop