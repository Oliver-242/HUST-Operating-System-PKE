#
# _mentry is the entry point of riscv-pke OS kernel.
#
# !Important (for your understanding)
# Before entering _mentry, two argument registers, i.e., a0(x10) and a1(x11), are set by
# our emulator (i.e., spike).
# [a0] = processor ID  (in the context of RISC-V, a processor is called as a HART, i.e.,
# Hardware Thread).
# [a1] = pointer to the DTS (i.e., Device Tree String), which is stored in the memory of
# RISC-V guest computer emulated by spike.
#

.globl _mentry
_mentry:
    # [mscratch] = 0; mscratch points the stack bottom of machine mode computer
    csrw mscratch, x0

    # following codes allocate a 4096-byte stack for each HART, although we use only
    # ONE HART in this lab.
    la sp, stack0		# stack0 is statically defined in kernel/machine/minit.c 
    li a3, 4096			# 4096-byte stack
    csrr a4, mhartid	# [mhartid] = core ID
    addi a4, a4, 1
    mul a3, a3, a4
    add sp, sp, a3		# re-arrange the stack points so that they don't overlap

    # jump to mstart(), i.e., machine state start function in kernel/machine/minit.c
    call m_start
