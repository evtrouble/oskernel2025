//
// Created by Li shuang ( pseudonym ) on 2024-03-26 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#pragma once 

extern "C" {
#include "asm.h"
#include "la_csr.h"
}

#include <kernel/types.hh>

namespace loongarch
{
	constexpr uint entry_stack_size = LOONGARCH_ENTRY_STACK_SIZE;

	void loongarch_init();

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
			crmd = _CSR_ENUM_NAME_( crmd ),       // Machine Status Register (mstatus in RISC-V)
			prmd = _CSR_ENUM_NAME_( prmd ),       // Supervisor Status Register (sstatus in RISC-V)
			euen = _CSR_ENUM_NAME_( euen ),       // Floating-Point Unit Enable (fcsr in RISC-V)
			misc = _CSR_ENUM_NAME_( misc ),       // Miscellaneous Control Register
			ecfg = _CSR_ENUM_NAME_( ecfg ),       // Exception Configuration Register (medeleg/mideleg in RISC-V)
			estat = _CSR_ENUM_NAME_( estat ),     // Exception Status Register (mcause/scause in RISC-V)
			era = _CSR_ENUM_NAME_( era ),         // Exception Return Address (mepc/sepc in RISC-V)
			badv = _CSR_ENUM_NAME_( badv ),       // Bad Virtual Address (mtval/stval in RISC-V)
			badi = _CSR_ENUM_NAME_( badi ),       // Bad Instruction Address
			eentry = _CSR_ENUM_NAME_( eentry ),   // Exception Entry Address (mtvec/stvec in RISC-V)
			tlbidx = _CSR_ENUM_NAME_( tlbidx ),   // TLB Index Register
			tlbehi = _CSR_ENUM_NAME_( tlbehi ),   // TLB Entry High Register
			tlbelo0 = _CSR_ENUM_NAME_( tlbelo0 ), // TLB Entry Low Register 0
			tlbelo1 = _CSR_ENUM_NAME_( tlbelo1 ), // TLB Entry Low Register 1
			asid = _CSR_ENUM_NAME_( asid ),       // Address Space Identifier
			pgdl = _CSR_ENUM_NAME_( pgdl ),       // Page Directory Base Register Low
			pgdh = _CSR_ENUM_NAME_( pgdh ),       // Page Directory Base Register High
			pgd = _CSR_ENUM_NAME_( pgd ),         // Page Directory Base Register
			pwcl = _CSR_ENUM_NAME_( pwcl ),       // Page Walk Control Low
			pwch = _CSR_ENUM_NAME_( pwch ),       // Page Walk Control High
			stlbps = _CSR_ENUM_NAME_( stlbps ),   // STLB Page Size
			rvacfg = _CSR_ENUM_NAME_( rvacfg ),   // RVAC Configuration
			cpuid = _CSR_ENUM_NAME_( cpuid ),     // CPU ID Register (mhartid in RISC-V)
			prcfg1 = _CSR_ENUM_NAME_( prcfg1 ),   // Processor Configuration 1
			prcfg2 = _CSR_ENUM_NAME_( prcfg2 ),   // Processor Configuration 2
			prcfg3 = _CSR_ENUM_NAME_( prcfg3 ),   // Processor Configuration 3
			save0 = _CSR_ENUM_NAME_( save0 ),     // Save Register 0
			tid = _CSR_ENUM_NAME_( tid ),         // Thread ID Register
			tcfg = _CSR_ENUM_NAME_( tcfg ),       // Timer Configuration Register (mtimecmp in RISC-V)
			tval = _CSR_ENUM_NAME_( tval ),       // Timer Value Register (mtime in RISC-V)
			cntc = _CSR_ENUM_NAME_( cntc ),       // Counter Control Register
			ticlr = _CSR_ENUM_NAME_( ticlr ),     // Timer Interrupt Clear Register
			llbctl = _CSR_ENUM_NAME_( llbctl ),   // Load-Linked/Store-Conditional Control Register
			impctl1 = _CSR_ENUM_NAME_( impctl1 ), // Implementation Control Register 1
			impctl2 = _CSR_ENUM_NAME_( impctl2 ), // Implementation Control Register 2
			tlbrentry = _CSR_ENUM_NAME_( tlbrentry ), // TLB Refill Entry Address
			tlbrbadv = _CSR_ENUM_NAME_( tlbrbadv ),   // TLB Refill Bad Virtual Address
			tlbrera = _CSR_ENUM_NAME_( tlbrera ),     // TLB Refill Exception Return Address
			tlbrsave = _CSR_ENUM_NAME_( tlbrsave ),   // TLB Refill Save Register
			tlbrelo0 = _CSR_ENUM_NAME_( tlbrelo0 ),   // TLB Refill Entry Low Register 0
			tlbrelo1 = _CSR_ENUM_NAME_( tlbrelo1 ),   // TLB Refill Entry Low Register 1
			tlbrehi = _CSR_ENUM_NAME_( tlbrehi ),     // TLB Refill Entry High Register
			tlbrprmd = _CSR_ENUM_NAME_( tlbrprmd ),   // TLB Refill Previous Mode Register
			merrctl = _CSR_ENUM_NAME_( merrctl ),     // Machine Error Control Register
			merrinfo1 = _CSR_ENUM_NAME_( merrinfo1 ), // Machine Error Information Register 1
			merrinfo2 = _CSR_ENUM_NAME_( merrinfo2 ), // Machine Error Information Register 2
			merrentry = _CSR_ENUM_NAME_( merrentry ), // Machine Error Entry Address
			merrera = _CSR_ENUM_NAME_( merrera ),     // Machine Error Exception Return Address
			merrsave = _CSR_ENUM_NAME_( merrsave ),   // Machine Error Save Register
			ctag = _CSR_ENUM_NAME_( ctag ),           // Cache Tag Register
			msgis0 = _CSR_ENUM_NAME_( msgis0 ),       // Message Interrupt Status Register 0
			msgis1 = _CSR_ENUM_NAME_( msgis1 ),       // Message Interrupt Status Register 1
			msgis2 = _CSR_ENUM_NAME_( msgis2 ),       // Message Interrupt Status Register 2
			msgis3 = _CSR_ENUM_NAME_( msgis3 ),       // Message Interrupt Status Register 3
			msgir = _CSR_ENUM_NAME_( msgir ),         // Message Interrupt Request Register
			msgie = _CSR_ENUM_NAME_( msgie ),         // Message Interrupt Enable Register
			dmwin0 = _CSR_ENUM_NAME_( dmwin0 ),       // Debug Mode Window 0
			dmwin1 = _CSR_ENUM_NAME_( dmwin1 ),       // Debug Mode Window 1
			dmwin2 = _CSR_ENUM_NAME_( dmwin2 ),       // Debug Mode Window 2
			dmwin3 = _CSR_ENUM_NAME_( dmwin3 ),       // Debug Mode Window 3
		};

#define _build_crmd_bit_( name, mask, shift ) \
	crmd_##name##_s = shift, \
	crmd_##name##_m = mask << crmd_##name##_s,
		enum Crmd : uint32
		{
			_build_crmd_bit_( plv, 0x3, 0 )
			_build_crmd_bit_( ie, 0x1, 2 )
			_build_crmd_bit_( da, 0x1, 3 )
			_build_crmd_bit_( pg, 0x1, 4 )
			_build_crmd_bit_( datf, 0x3, 5 )
			_build_crmd_bit_( datm, 0x3, 7 )
			_build_crmd_bit_( we, 0x1, 9 )
		};
#undef _build_crmd_bit_

#define _build_prmd_bit_( name, mask, shift ) \
	prmd_##name##_s = shift, \
	prmd_##name##_m = mask << prmd_##name##_s,
		enum Prmd : uint32
		{
			_build_prmd_bit_( pplv, 0x3U, 0 )
			_build_prmd_bit_( pie, 0x1U, 2 )
			_build_prmd_bit_( pwe, 0x1U, 3 )
			_build_prmd_bit_( resv, 0xFFFFFFFU, 4 )	// reserved
		};
#undef _build_prmd_bit_

#define _build_euen_bit_( name, mask, shift ) \
	euen_##name##_s = shift, \
	euen_##name##_m = mask << euen_##name##_s,
		enum Euen : uint32
		{
			_build_euen_bit_( fpe, 0x1, 0 )
			_build_euen_bit_( sxe, 0x1, 1 )
			_build_euen_bit_( asxe, 0x1, 2 )
			_build_euen_bit_( bte, 0x1, 3 )
		};
#undef _build_euen_bit_

		/// @brief s: shift;
		///		   m: mask;	
		enum Tcfg : uint64
		{
			tcfg_en_s = 0,
			tcfg_en_m = 0x1UL << tcfg_en_s,
			tcfg_periodic_s = 1,
			tcfg_periodic_m = 0x1UL << tcfg_periodic_s,
			tcfg_initval_s = 2,
			tcfg_initval_m = 0x3fffffffffffffffUL << tcfg_initval_s
		};

		/// @brief 13个中断源的枚举
		/// s: shift;
		/// m: mask;
		enum itr : uint64
		{
			itr_ipi_s = 12,
			itr_ti_s = 11,
			itr_pmi_s = 10,
			itr_hwi7_s = 9,
			itr_hwi6_s = 8,
			itr_hwi5_s = 7,
			itr_hwi4_s = 6,
			itr_hwi3_s = 5,
			itr_hwi2_s = 4,
			itr_hwi1_s = 3,
			itr_hwi0_s = 2,
			itr_hwi_s = itr_hwi0_s,
			itr_swi1_s = 1,
			itr_swi0_s = 0,
			itr_swi_s = itr_swi0_s,

			itr_ipi_m = 0x1 << itr_ipi_s,
			itr_ti_m = 0x1 << itr_ti_s,
			itr_pmi_m = 0x1 << itr_pmi_s,
			itr_hwi7_m = 0x1 << itr_hwi7_s,
			itr_hwi6_m = 0x1 << itr_hwi6_s,
			itr_hwi5_m = 0x1 << itr_hwi5_s,
			itr_hwi4_m = 0x1 << itr_hwi4_s,
			itr_hwi3_m = 0x1 << itr_hwi3_s,
			itr_hwi2_m = 0x1 << itr_hwi2_s,
			itr_hwi1_m = 0x1 << itr_hwi1_s,
			itr_hwi0_m = 0x1 << itr_hwi0_s,
			itr_hwi_m = 0xff << itr_hwi_s,
			itr_swi1_m = 0x1 << itr_swi1_s,
			itr_swi0_m = 0x1 << itr_swi0_s,
			itr_swi_m = 0x3 << itr_swi_s,
		};

		/// @brief 例外前模式信息
		enum prmd : uint64
		{
			pplv_s = 0,
			pie_s = 2,
			pwe_s = 3,

			pplv_m = 0x3 << pplv_s,
			pie_m = 0x1 << pie_s,
			pwe_m = 0x1 << pwe_s,
		};

		/// @brief s: shift;
		///		   m: mask;
		enum ecfg : uint64
		{
			ecfg_lie_s = 0,
			ecfg_vs_s = 16,

			ecfg_lie_m = 0x1FFF << ecfg_lie_s,
			ecfg_vs_m = 0x7 << ecfg_vs_s,
		};

#define _build_estat_bit_( name, mask, shift ) \
	estat_##name##_s = shift, \
	estat_##name##_m = mask << estat_##name##_s, 
		enum Estat : uint32
		{
			_build_estat_bit_( is, 0x1FFFU, 0 )
			_build_estat_bit_( is_swi0, 0x1U, 0 )
			_build_estat_bit_( is_swi1, 0x1U, 1 )
			_build_estat_bit_( is_hwi0, 0x1U, 2 )
			_build_estat_bit_( is_hwi1, 0x1U, 3 )
			_build_estat_bit_( is_hwi2, 0x1U, 4 )
			_build_estat_bit_( is_hwi3, 0x1U, 5 )
			_build_estat_bit_( is_hwi4, 0x1U, 6 )
			_build_estat_bit_( is_hwi5, 0x1U, 7 )
			_build_estat_bit_( is_hwi6, 0x1U, 8 )
			_build_estat_bit_( is_hwi7, 0x1U, 9 )
			_build_estat_bit_( is_pmi, 0x1U, 10 )
			_build_estat_bit_( is_ti, 0x1U, 11 )
			_build_estat_bit_( is_ipi, 0x1U, 12 )
			_build_estat_bit_( msgint, 0x1U, 14 )
			_build_estat_bit_( ecode, 0x3FU, 16 )
			_build_estat_bit_( esubcode, 0x1FFU, 22 )
		};
#undef _build_estat_bit_ 

#define _build_ecode_enum_(name,code) \
	ecode_##name = code, 
		enum Ecode : uint
		{
			_build_ecode_enum_( int, 0x0 )
			_build_ecode_enum_( pil, 0x1 )
			_build_ecode_enum_( pis, 0x2 )
			_build_ecode_enum_( pif, 0x3 )
			_build_ecode_enum_( pme, 0x4 )
			_build_ecode_enum_( pnr, 0x5 )
			_build_ecode_enum_( pnx, 0x6 )
			_build_ecode_enum_( ppi, 0x7 )
			_build_ecode_enum_( ade, 0x8 )
			_build_ecode_enum_( ale, 0x9 )
			_build_ecode_enum_( bce, 0xA )
			_build_ecode_enum_( sys, 0xB )
			_build_ecode_enum_( brk, 0xC )
			_build_ecode_enum_( ine, 0xD )
			_build_ecode_enum_( ipe, 0xE )
			_build_ecode_enum_( fpd, 0xF )
			_build_ecode_enum_( sxd, 0x10 )
			_build_ecode_enum_( asxd, 0x11 )
			_build_ecode_enum_( fpe, 0x12 )
			_build_ecode_enum_( wpe, 0x13 )
			_build_ecode_enum_( btd, 0x14 )
			_build_ecode_enum_( bte, 0x15 )
			_build_ecode_enum_( gspr, 0x16 )
			_build_ecode_enum_( hvc, 0x17 )
			_build_ecode_enum_( gcc, 0x18 )
		};
#undef _build_ecode_enum_

#define _build_pwcl_bit_( name, mask, shift ) \
	pwcl_##name##_s = shift, \
	pwcl_##name##_m = mask << pwcl_##name##_s,
		enum Pwcl : uint32
		{
			_build_pwcl_bit_( pt_base, 0x5UL, 0 )
			_build_pwcl_bit_( pt_width, 0x5UL, 5 )
			_build_pwcl_bit_( dir1_base, 0x5UL, 10 )
			_build_pwcl_bit_( dir1_width, 0x5UL, 15 )
			_build_pwcl_bit_( dir2_base, 0x5UL, 20 )
			_build_pwcl_bit_( dir2_width, 0x5UL, 25 )
			_build_pwcl_bit_( pte_width, 0x2UL, 30 )

			pwcl_pte_width_64bit = 0,
			pwcl_pte_width_128bit = 1,
			pwcl_pte_width_256bit = 2,
			pwcl_pte_width_512bit = 3,
		};
#undef _build_pwcl_bit_
		
#define _build_pwch_bit_( name, mask, shift ) \
	pwch_##name##_s = shift, \
	pwch_##name##_m = mask << pwch_##name##_s,
		enum Pwch : uint32
		{
			_build_pwch_bit_( dir3_base, 0x6UL, 0 )
			_build_pwch_bit_( dir3_width, 0x6UL, 6 )
			_build_pwch_bit_( dir4_base, 0x6UL, 12 )
			_build_pwch_bit_( dir4_width, 0x6UL, 18 )
			_build_pwch_bit_( hptw_en, 0x1UL, 24 )
		};
#undef _build_pwch_bit_

		static inline uint64 _read_csr_( CsrAddr _csr )
		{
			return _la_r_csr_[ ( uint64 ) _csr ]();
		}

		static inline void _write_csr_( CsrAddr _csr, uint64 _data )
		{
			return _la_w_csr_[ ( uint64 ) _csr ]( _data );
		}

	} // namespace csr

} // namespace loongarch
