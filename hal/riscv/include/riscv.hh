#pragma once 
#include <kernel/types.hh>

namespace riscv
{
	#define RISCV_ENTRY_STACK_SIZE		0x4000	/* 16 KiB */
	constexpr uint entry_stack_size = RISCV_ENTRY_STACK_SIZE;

	void riscv_init();

#define _build_pte_bit_(name,mask,shift) \
	pte_##name##_s = shift, \
	pte_##name##_m = mask << pte_##name##_s,
	// 页表项各个位的作用说明:
	// [0]     V (Valid): 页表项是否有效
	// [1]     R (Read): 页表项是否可读
	// [2]     W (Write): 页表项是否可写
	// [3]     X (Execute): 页表项是否可执行
	// [4]     U (User): 页表项是否用户模式可访问
	// [5]     G (Global): 页表项是否全局映射
	// [6]     A (Accessed): 页表项是否被访问过
	// [7]     D (Dirty): 页表项是否被写过
	// [8-9]   RSW (Reserved for Software): 保留给软件使用
	// [10-53] PPN (Physical Page Number): 物理页号
	// [54-63] Reserved: 保留位
	enum PteEnum : uint64
	{
		_build_pte_bit_( valid, 0x1, 0 )
		_build_pte_bit_( read, 0x1, 1 )
		_build_pte_bit_( write, 0x1, 2 )
		_build_pte_bit_( execute, 0x1, 3 )
		_build_pte_bit_( user, 0x1, 4 )
		_build_pte_bit_( global, 0x1, 5 )
		_build_pte_bit_( accessed, 0x1, 6 )
		_build_pte_bit_( dirty, 0x1, 7 )
		_build_pte_bit_( rsw, 0x3, 8 )
		_build_pte_bit_( ppn, 0xFFFFFFFFFFFUL, 10 )
		_build_pte_bit_( reserved, 0x3FFUL, 54 )

		pte_flags_m = pte_valid_m | pte_read_m | pte_write_m | pte_execute_m | pte_user_m | pte_global_m
		| pte_accessed_m | pte_dirty_m | pte_rsw_m
	};
#undef _build_pte_bit_

	enum PlvEnum : uint
	{
		plv_super = 0,
		plv_user = 3
	};

	namespace csr
	{
		enum CsrAddr : uint64
		{
			// Machine Information Registers
			mvendorid = 0xF11,  // Vendor ID
			marchid = 0xF12,    // Architecture ID
			mimpid = 0xF13,     // Implementation ID
			mhartid = 0xF14,    // Hardware Thread ID (cpuid in LoongArch)

			// Machine Trap Setup
			mstatus = 0x300,    // Machine Status Register (crmd in LoongArch)
			misa = 0x301,       // ISA and Extensions
			medeleg = 0x302,    // Machine Exception Delegation (ecfg in LoongArch)
			mideleg = 0x303,    // Machine Interrupt Delegation (ecfg in LoongArch)
			mie = 0x304,        // Machine Interrupt Enable
			mtvec = 0x305,      // Machine Trap-Vector Base Address (eentry in LoongArch)
			time = 0xC01,       // Timer Register
			timecmp = 0xC02,    // Timer Compare Register
			mscratch = 0x340,   // Machine Scratch Register
			mepc = 0x341,       // Machine Exception Program Counter (era in LoongArch)
			mcause = 0x342,     // Machine Cause Register (estat in LoongArch)
			mtval = 0x343,      // Machine Trap Value Register (badv in LoongArch)
			mip = 0x344,        // Machine Interrupt Pending

			// Supervisor Trap Setup
			sstatus = 0x100,    // Supervisor Status Register (prmd in LoongArch)
			sedeleg = 0x102,    // Supervisor Exception Delegation
			sideleg = 0x103,    // Supervisor Interrupt Delegation
			sie = 0x104,        // Supervisor Interrupt Enable
			stvec = 0x105,      // Supervisor Trap-Vector Base Address (eentry in LoongArch)
			sscratch = 0x140,   // Supervisor Scratch Register
			sepc = 0x141,       // Supervisor Exception Program Counter (era in LoongArch)
			scause = 0x142,     // Supervisor Cause Register (estat in LoongArch)
			stval = 0x143,      // Supervisor Trap Value Register (badv in LoongArch)
			sip = 0x144,        // Supervisor Interrupt Pending
			satp = 0x180,       // Supervisor Address Translation and Protection

			// User Trap Setup
			ustatus = 0x000,    // User Status Register
			uie = 0x004,        // User Interrupt Enable
			utvec = 0x005,      // User Trap-Vector Base Address
			uscratch = 0x040,   // User Scratch Register
			uepc = 0x041,       // User Exception Program Counter
			ucause = 0x042,     // User Cause Register
			utval = 0x043,      // User Trap Value Register
			uip = 0x044,        // User Interrupt Pending
		};

#define _build_mstatus_bit_( name, mask, shift ) \
	mstatus_##name##_s = shift, \
	mstatus_##name##_m = mask << mstatus_##name##_s,
		enum Mstatus : uint32
		{
			_build_mstatus_bit_( sie, 0x1, 1 )
			_build_mstatus_bit_( mie, 0x1, 3 )
			_build_mstatus_bit_( spie, 0x1, 5 )
			_build_mstatus_bit_( mpie, 0x1, 7 )
			_build_mstatus_bit_( spp, 0x1, 8 )
			_build_mstatus_bit_( mpp, 0x3, 11 )
			_build_mstatus_bit_( fs, 0x3, 13 )
			_build_mstatus_bit_( xs, 0x3, 15 )
			_build_mstatus_bit_( mprv, 0x1, 17 )
			_build_mstatus_bit_( sum, 0x1, 18 )
			_build_mstatus_bit_( mxr, 0x1, 19 )
			_build_mstatus_bit_( tvm, 0x1, 20 )
			_build_mstatus_bit_( tw, 0x1, 21 )
			_build_mstatus_bit_( tsr, 0x1, 22 )
		};
#undef _build_mstatus_bit_

#define _build_mie_bit_( name, mask, shift ) \
	mie_##name##_s = shift, \
	mie_##name##_m = mask << mie_##name##_s,
		enum Mie : uint32
		{
			_build_mie_bit_( ssie, 0x1, 1 )
			_build_mie_bit_( msie, 0x1, 3 )
			_build_mie_bit_( stie, 0x1, 5 )
			_build_mie_bit_( mtie, 0x1, 7 )
			_build_mie_bit_( seie, 0x1, 9 )
			_build_mie_bit_( meie, 0x1, 11 )
		};
#undef _build_mie_bit_

#define _build_mip_bit_( name, mask, shift ) \
	mip_##name##_s = shift, \
	mip_##name##_m = mask << mip_##name##_s,
		enum Mip : uint32
		{
			_build_mip_bit_( ssip, 0x1, 1 )
			_build_mip_bit_( msip, 0x1, 3 )
			_build_mip_bit_( stip, 0x1, 5 )
			_build_mip_bit_( mtip, 0x1, 7 )
			_build_mip_bit_( seip, 0x1, 9 )
			_build_mip_bit_( meip, 0x1, 11 )
		};
#undef _build_mip_bit_

#define _build_sstatus_bit_( name, mask, shift ) \
	sstatus_##name##_s = shift, \
	sstatus_##name##_m = mask << sstatus_##name##_s,
		enum Sstatus : uint32
		{
			_build_sstatus_bit_( sie, 0x1, 1 )
			_build_sstatus_bit_( spie, 0x1, 5 )
			_build_sstatus_bit_( spp, 0x1, 8 )
			_build_sstatus_bit_( fs, 0x3, 13 )
			_build_sstatus_bit_( xs, 0x3, 15 )
			_build_sstatus_bit_( sum, 0x1, 18 )
			_build_sstatus_bit_( mxr, 0x1, 19 )
		};
#undef _build_sstatus_bit_

#define _build_sie_bit_( name, mask, shift ) \
	sie_##name##_s = shift, \
	sie_##name##_m = mask << sie_##name##_s,
		enum Sie : uint32
		{
			_build_sie_bit_( ssie, 0x1, 1 )
			_build_sie_bit_( stie, 0x1, 5 )
			_build_sie_bit_( seie, 0x1, 9 )
		};
#undef _build_sie_bit_

#define _build_sip_bit_( name, mask, shift ) \
	sip_##name##_s = shift, \
	sip_##name##_m = mask << sip_##name##_s,
		enum Sip : uint32
		{
			_build_sip_bit_( ssip, 0x1, 1 )
			_build_sip_bit_( stip, 0x1, 5 )
			_build_sip_bit_( seip, 0x1, 9 )
		};
#undef _build_sip_bit_

#define _build_ustatus_bit_( name, mask, shift ) \
	ustatus_##name##_s = shift, \
	ustatus_##name##_m = mask << ustatus_##name##_s,
		enum Ustatus : uint32
		{
			_build_ustatus_bit_( uie, 0x1, 0 )
			_build_ustatus_bit_( upie, 0x1, 4 )
		};
#undef _build_ustatus_bit_

#define _build_uie_bit_( name, mask, shift ) \
	uie_##name##_s = shift, \
	uie_##name##_m = mask << uie_##name##_s,
		enum Uie : uint32
		{
			_build_uie_bit_( usie, 0x1, 0 )
			_build_uie_bit_( utie, 0x1, 4 )
			_build_uie_bit_( ueie, 0x1, 8 )
		};
#undef _build_uie_bit_

#define _build_uip_bit_( name, mask, shift ) \
	uip_##name##_s = shift, \
	uip_##name##_m = mask << uip_##name##_s,
		enum Uip : uint32
		{
			_build_uip_bit_( usip, 0x1, 0 )
			_build_uip_bit_( utip, 0x1, 4 )
			_build_uip_bit_( ueip, 0x1, 8 )
		};
#undef _build_uip_bit_

		static inline uint64 _read_csr_( CsrAddr _csr )
		{
			uint64 value;
			asm volatile ("csrr %0, %1" : "=r"(value) : "i"(_csr));
			return value;
		}

		static inline void _write_csr_( CsrAddr _csr, uint64 _data )
		{
			asm volatile ("csrw %0, %1" : : "i"(_csr), "r"(_data));
		}

		static inline void _set_csr_( CsrAddr _csr, uint64 _data )
		{
			asm volatile ("csrs %0, %1" : : "i"(_csr), "r"(_data));
		}

		static inline void _clear_csr_( CsrAddr _csr, uint64 _data )
		{
			asm volatile ("csrc %0, %1" : : "i"(_csr), "r"(_data));
		}

	} // namespace csr

} // namespace riscv
