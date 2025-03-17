    .section .text.osstartup
    .globl _entry
_entry:
    la sp, boot_stack
    mv tp, a0
    call _cpu_init

	li fp, 0
    la gp, __global_pointer$
    call xn6_start_kernel

loop:
    j loop

    .section .bss.stack
    .align 12
    .globl boot_stack
boot_stack:
    .space 4096 * 4 * 2
    .globl boot_stack_top
boot_stack_top: