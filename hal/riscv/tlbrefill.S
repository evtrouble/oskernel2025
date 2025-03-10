#include "asm.h"

.section .text.tlbrentry
.globl handle_tlbr
.align 4
handle_tlbr:
	csrrw	t0, RISCV_CSR_MSCRATCH, t0  # 保存当前TLB重填状态
	csrr	t0, RISCV_CSR_MSTATUS  # 读取异常原因寄存器
	csrr	t0, RISCV_CSR_SATP  # 读取页表基地址寄存器
	ld	t0, 0(t0)  # 读取页表条目
	srli	t0, t0, 12  # 地址右移12位
	slli	t0, t0, 12  # 地址左移12位
	ld	t0, 0(t0)  # 读取页表条目
	srli	t0, t0, 12  # 地址右移12位
	slli	t0, t0, 12  # 地址左移12位
	ld	t0, 0(t0)  # 读取页表条目
	srli	t0, t0, 12  # 地址右移12位
	slli	t0, t0, 12  # 地址左移12位
	sd	t0, 0(t0)  # 加载页表条目到TLB
	csrr	t0, RISCV_CSR_MSCRATCH  # 恢复TLB重填状态
	mret  # 返回到异常发生前的状态
