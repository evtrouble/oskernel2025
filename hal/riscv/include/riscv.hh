#pragma once 

namespace riscv
{
	#define RISCV_ENTRY_STACK_SIZE		0x4000	/* 16 KiB */
	constexpr uint entry_stack_size = RISCV_ENTRY_STACK_SIZE;

	// 定义适用于四级页表的 SATP 模式
	#define SATP_SV48 (9L << 60)
	#define SATP_SV39 (8L << 60)
		
	// 三级页表
	#define MAKE_SATP(pagetable) (SATP_SV39 | (((uint64)pagetable) >> 12))

	// Sv48
	// #define MAXVA (1L << (9 + 9 + 9 + 9 + 12 - 1))

	// Sv39
	#define MAXVA (1L << (9 + 9 + 9 + 12 - 1))

	// map the trampoline page to the highest address,
	// in both user and kernel space.
	#define TRAMPOLINE              (MAXVA - PG_SIZE)

	void riscv_init();
	// supervisor-mode cycle counter
	inline uint64 r_time()
	{
		uint64 x;
		// asm volatile("csrr %0, time" : "=r" (x) );
		// this instruction will trap in SBI
		asm volatile("rdtime %0" : "=r" (x) );
		return x;
	}

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
				// Supervisor Trap Setup
				sstatus = 0x100,    // Supervisor Status Register (prmd in LoongArch)
				// sedeleg = 0x102,    // Supervisor Exception Delegation
				// sideleg = 0x103,    // Supervisor Interrupt Delegation
				sie = 0x104,        // Supervisor Interrupt Enable
				stvec = 0x105,      // Supervisor Trap-Vector Base Address (eentry in LoongArch)
				// sscratch = 0x140,   // Supervisor Scratch Register
				sepc = 0x141,       // Supervisor Exception Program Counter (era in LoongArch)
				scause = 0x142,     // Supervisor Cause Register (estat in LoongArch)
				stval = 0x143,      // Supervisor Trap Value Register (badv in LoongArch)
				sip = 0x144,        // Supervisor Interrupt Pending
				satp = 0x180,       // Supervisor Address Translation and Protection

				// User Trap Setup
				// ustatus = 0x000,    // User Status Register
				// uie = 0x004,        // User Interrupt Enable
				// utvec = 0x005,      // User Trap-Vector Base Address
				// uscratch = 0x040,   // User Scratch Register
				// uepc = 0x041,       // User Exception Program Counter
				// ucause = 0x042,     // User Cause Register
				// utval = 0x043,      // User Trap Value Register
				// uip = 0x044,        // User Interrupt Pending
			};

	#define _build_sstatus_bit_( name, mask, shift ) \
		sstatus_##name##_s = shift, \
		sstatus_##name##_m = mask << sstatus_##name##_s,
			enum Sstatus : uint32
			{
				_build_sstatus_bit_( uie, 0x1, 0 )
				_build_sstatus_bit_( sie, 0x1, 1 )
				_build_sstatus_bit_( upie, 0x1, 4 )
				_build_sstatus_bit_( spie, 0x1, 5 )
				_build_sstatus_bit_( spp, 0x1, 8 )
				_build_sstatus_bit_( fs, 0x3, 13 )
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

			#define DEFINE_CSR_OPS(name) \
			static inline uint64 read_##name() { \
				uint64 value; \
				asm volatile("csrr %0, " #name : "=r"(value)); \
				return value; \
			} \
			static inline void write_##name(uint64 value) { \
				asm volatile("csrw " #name ", %0" : : "r"(value)); \
			} \
			static inline void set_##name(uint64 value) { \
				asm volatile("csrs " #name ", %0" : : "r"(value)); \
			} \
			static inline void clear_##name(uint64 value) { \
				asm volatile("csrc " #name ", %0" : : "r"(value)); \
			}

		// 为每个CSR生成操作函数
		DEFINE_CSR_OPS(sstatus)
		DEFINE_CSR_OPS(stvec)
		DEFINE_CSR_OPS(sepc)
		DEFINE_CSR_OPS(scause)
		DEFINE_CSR_OPS(sie)
		DEFINE_CSR_OPS(sip)
		DEFINE_CSR_OPS(satp)
		DEFINE_CSR_OPS(stval)
		// ...other CSRs...

		#undef DEFINE_CSR_OPS

		// 通用CSR操作函数
		static inline uint64 _read_csr_(CsrAddr _csr) {
			switch(_csr) {
				case sstatus: return read_sstatus();
				case stvec:   return read_stvec();
				case sepc:    return read_sepc();
				case scause:  return read_scause();
				case sie:     return read_sie();
				case sip:     return read_sip();
				case satp:    return read_satp();
				case stval:   return read_stval();
				// ...other cases...
				default:      return 0;
			}
		}

		static inline void _write_csr_(CsrAddr _csr, uint64 _data) {
			switch(_csr) {
				case sstatus: write_sstatus(_data); break;
				case stvec:   write_stvec(_data);   break;
				case sepc:    write_sepc(_data);    break;
				case scause:  write_scause(_data);  break;
				case sie:     write_sie(_data);     break;
				case sip:     write_sip(_data);     break;
				case satp:    write_satp(_data);    break;
				case stval:   write_stval(_data);   break;
				// ...other cases...
			}
		}

		static inline void _set_csr_(CsrAddr _csr, uint64 _data) {
			switch(_csr) {
				case sstatus: set_sstatus(_data); break;
				case stvec:   set_stvec(_data);   break;
				case sepc:    set_sepc(_data);    break;
				case scause:  set_scause(_data);  break;
				case sie:     set_sie(_data);     break;
				case sip:     set_sip(_data);     break;
				case satp:    set_satp(_data);    break;
				case stval:   set_stval(_data);   break;
				// ...other cases...
			}
		}

		static inline void _clear_csr_(CsrAddr _csr, uint64 _data) {
			switch(_csr) {
				case sstatus: clear_sstatus(_data); break;
				case stvec:   clear_stvec(_data);   break;
				case sepc:    clear_sepc(_data);    break;
				case scause:  clear_scause(_data);  break;
				case sie:     clear_sie(_data);     break;
				case sip:     clear_sip(_data);     break;
				case satp:    clear_satp(_data);    break;
				case stval:   clear_stval(_data);   break;
				// ...other cases...
			}
		}

	} // namespace csr

} // namespace riscv
