#include "util/load_store.S"

#
# M-mode trap entry point
#
.globl mtrapvec
.align 4
mtrapvec:
    # mscratch -> g_itrframe (cf. kernel/machine/minit.c line 94)
    # swap a0 and mscratch, so that a0 points to interrupt frame,
    # i.e., [a0] = &g_itrframe
    csrrw a0, mscratch, a0

    # save the registers in g_itrframe
    addi t6, a0, 0
    store_all_registers
    # save the original content of a0 in g_itrframe
    csrr t0, mscratch
    sd t0, 72(a0)

    # switch stack (to use stack0) for the rest of machine mode
    # trap handling.
    la sp, stack0
    li a3, 4096
    csrr a4, mhartid
    addi a4, a4, 1
    mul a3, a3, a4
    add sp, sp, a3

    # pointing mscratch back to g_itrframe
    csrw mscratch, a0

    # call machine mode trap handling function
    call handle_mtrap

    # restore all registers, come back to the status before entering
    # machine mode handling.
    csrr t6, mscratch
    restore_all_registers

    mret
