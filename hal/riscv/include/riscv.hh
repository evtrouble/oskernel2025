#pragma once 

extern "C" {
#include "asm.h"
}

#include <kernel/types.hh>

namespace riscv
{
	constexpr uint entry_stack_size = RISCV_ENTRY_STACK_SIZE;

	void riscv_init();

	enum PgEnum : uint64
	{
		pg_flags_mask = 0xE000'0000'0000'01FFUL,
	};

	constexpr uint64 dmwin_mask = 0xFUL << 60;
	enum dmwin : uint64
	{
		win_0 = 0x9UL << 60,
		win_1 = 0x8UL << 60,
	};
	constexpr uint64 virt_to_phy_address( uint64 virt ) { return virt & ~dmwin_mask; }

#define _build_pte_bit_(name,mask,shift) \
	pte_##name##_s = shift, \
	pte_##name##_m = mask << pte_##name##_s,
	enum PteEnum : uint64
	{
		_build_pte_bit_( valid, 0x1, 0 )
		_build_pte_bit_( dirty, 0x1, 1 )
		_build_pte_bit_( plv, 0x3, 2 )
		_build_pte_bit_( mat, 0x3, 4 )
		_build_pte_bit_( base_global, 0x1, 6 )
		_build_pte_bit_( dir_huge, 0x1, 6 )
		_build_pte_bit_( present, 0x1, 7 )
		_build_pte_bit_( writable, 0x1, 8 )
		_build_pte_bit_( base_pa, 0x1FFFFFFFFFFFFUL, 12 )
		_build_pte_bit_( huge_global, 0x1, 12 )
		_build_pte_bit_( no_read, 0x1UL, 61 )
		_build_pte_bit_( no_execute, 0x1UL, 62 )
		_build_pte_bit_( rplv, 0x1UL, 63 )

		pte_flags_m = pte_valid_m | pte_dirty_m | pte_plv_m | pte_mat_m | pte_base_global_m | pte_present_m
		| pte_writable_m | pte_no_read_m | pte_no_execute_m | pte_rplv_m,
	};
#undef _build_pte_bit_
	static_assert( PteEnum::pte_flags_m == 0xE0000000000001FFUL );

	enum MatEnum : uint
	{
		mat_suc = 0x0,			// 强序非缓存
		mat_cc = 0x1,			// 一致可缓存
		mat_wuc = 0x2,			// 弱序非缓存
		mat_undefined = 0x3
	};

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
				mhartid = 0xF14,    // Hardware Thread ID

				// Machine Trap Setup
				mstatus = 0x300,    // Machine Status Register
				misa = 0x301,       // ISA and Extensions
				medeleg = 0x302,    // Machine Exception Delegation
				mideleg = 0x303,    // Machine Interrupt Delegation
				mie = 0x304,        // Machine Interrupt Enable
				mtvec = 0x305,      // Machine Trap-Vector Base Address
				mscratch = 0x340,   // Machine Scratch Register
				mepc = 0x341,       // Machine Exception Program Counter
				mcause = 0x342,     // Machine Cause Register
				mtval = 0x343,      // Machine Trap Value Register
				mip = 0x344,        // Machine Interrupt Pending

				// Supervisor Trap Setup
				sstatus = 0x100,    // Supervisor Status Register
				sedeleg = 0x102,    // Supervisor Exception Delegation
				sideleg = 0x103,    // Supervisor Interrupt Delegation
				sie = 0x104,        // Supervisor Interrupt Enable
				stvec = 0x105,      // Supervisor Trap-Vector Base Address
				sscratch = 0x140,   // Supervisor Scratch Register
				sepc = 0x141,       // Supervisor Exception Program Counter
				scause = 0x142,     // Supervisor Cause Register
				stval = 0x143,      // Supervisor Trap Value Register
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
			_build_mstatus_bit_( uxl, 0x3, 32 )
			_build_mstatus_bit_( sxl, 0x3, 34 )
			_build_mstatus_bit_( sd, 0x1, 63 )
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
			_build_sstatus_bit_( uxl, 0x3, 32 )
			_build_sstatus_bit_( sd, 0x1, 63 )
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

	} // namespace csr

} // namespace riscv
