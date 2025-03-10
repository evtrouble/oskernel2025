//
// Created by Li shuang ( pseudonym ) on 2024-04-05 
// --------------------------------------------------------------
// | Note: This code file just for study, not for commercial use 
// | Contact Author: lishuang.mk@whu.edu.cn 
// --------------------------------------------------------------
//

#include "asm.h"

.section .text.tlbrentry
.globl handle_tlbr
.align 0x4
handle_tlbr:
	csrwr	$t0, LOONGARCH_CSR_TLBRSAVE  # 保存当前TLB重填状态
	csrrd	$t0, 0x5	# read estat  # 读取异常状态寄存器
	csrrd	$t0, LOONGARCH_CSR_PGD  # 读取页全局目录
	lddir	$t0, $t0, 3  # 读取第三级页表条目
	srli.d	$t0, $t0, 12  # 地址右移12位
	slli.d	$t0, $t0, 12  # 地址左移12位
	lddir	$t0, $t0, 2  # 读取第二级页表条目
	srli.d	$t0, $t0, 12  # 地址右移12位
	slli.d	$t0, $t0, 12  # 地址左移12位
	lddir	$t0, $t0, 1  # 读取第一级页表条目
	srli.d	$t0, $t0, 12  # 地址右移12位
	slli.d	$t0, $t0, 12  # 地址左移12位
	ldpte	$t0, 0  # 加载页表条目到TLB
	ldpte	$t0, 1  # 加载页表条目到TLB
	csrrd	$t0, 0x8c  # 读取CSR寄存器
	csrrd	$t0, 0x8d  # 读取CSR寄存器
	tlbfill  # 填充TLB
	csrrd	$t0, LOONGARCH_CSR_TLBRSAVE  # 恢复TLB重填状态
	ertn  # 返回到异常发生前的状态

