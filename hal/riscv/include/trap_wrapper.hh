#pragma once 

extern "C" {
	extern void _wrp_kernel_trap( void );
	extern void _wrp_machine_trap( void );
	extern void _wrp_user_trap();
	extern void _wrp_user_trap_ret( void );
}