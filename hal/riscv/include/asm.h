// LoongArch: LOONGARCH_CSR_CRMD
// RISC-V: CSR_MSTATUS (Machine Status Register)
#define RISCV_CSR_MSTATUS           0x300

// LoongArch: LOONGARCH_CSR_CPUID
// RISC-V: CSR_MHARTID (Hart ID Register)
#define RISCV_CSR_MHARTID           0xF14

// LoongArch: LOONGARCH_CSR_SAVE0
// RISC-V: CSR_MSCRATCH (Scratch Register)
#define RISCV_CSR_MSCRATCH          0x340

// LoongArch: LOONGARCH_CSR_DMWIN0
// RISC-V: CSR_PMPADDR0 (Physical Memory Protection Address Register)
#define RISCV_CSR_PMPADDR0          0x3B0

// LoongArch: LOONGARCH_CSR_TLBEHI
// RISC-V: CSR_SATP (Supervisor Address Translation and Protection)
#define RISCV_CSR_SATP              0x180

#define RISCV_ENTRY_STACK_SIZE		0x4000	/* 16 KiB */