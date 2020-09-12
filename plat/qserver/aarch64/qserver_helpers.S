/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <cortex_a53.h>
#include <asm_macros.S>
#include "../qserver_def.h"

	.globl	plat_qserver_calc_core_pos
	.globl	plat_my_core_pos
	.globl	platform_mem_init
	.globl	plat_crash_console_init
	.globl	plat_crash_console_putc
	.globl	plat_crash_console_flush
	.globl	plat_report_exception
	.globl	plat_reset_handler
	.globl	plat_is_my_cpu_primary
	.globl	plat_secondary_cold_boot_setup

func plat_my_core_pos
	mrs	x0, mpidr_el1
	b	plat_qserver_calc_core_pos
endfunc plat_my_core_pos

	/* -----------------------------------------------------
	 *  unsigned int plat_arm_calc_core_pos(u_register_t mpidr)
	 *  Helper function to calculate the core position.
	 *  With this function: CorePos = (ClusterId * 4) +
	 *  				  CoreId
	 * -----------------------------------------------------
	 */
func plat_qserver_calc_core_pos
        mrs	x3, mpidr_el1
        tst     x3, #MPIDR_MT_MASK
        beq     1f
	lsr	x0, x0, #8
1:
        mov     x3, x0
        /* Extract individual affinity fields from MPIDR */
	ubfx	x0, x3, #MPIDR_AFF0_SHIFT, #MPIDR_AFFINITY_BITS
	ubfx	x1, x3, #MPIDR_AFF1_SHIFT, #MPIDR_AFFINITY_BITS
	/*ubfx	x2, x3, #MPIDR_AFF2_SHIFT, #MPIDR_AFFINITY_BITS*/
	/* Compute linear position */
	mov	x4, #PLATFORM_CORE_COUNT_PER_CLUSTER
	madd	x0, x1, x4, x0
	/*mov	x5, #FVP_MAX_PE_PER_CPU*/
	/*madd	x0, x1, x5, x0*/
	ret
endfunc plat_qserver_calc_core_pos
	/* -----------------------------------------------------
	 * unsigned int plat_is_my_cpu_primary (void);
	 *
	 * Find out whether the current cpu is the primary
	 * cpu.
	 * -----------------------------------------------------
	 */
func plat_is_my_cpu_primary
	mov_imm	x0, PLAT_QSERVER_TRUSTED_MAILBOX_BASE
	str xzr, [x0]
	mrs	x0, mpidr_el1
        lsr	x0, x0, #8
	and	x0, x0, #(MPIDR_CLUSTER_MASK | MPIDR_CPU_MASK)
        /*mov_imm x1, MPIDR_AFFINITY_MASK
        and     x0, x0, x1*/
	cmp	x0, #QSERVER_PRIMARY_CPU
	cset	w0, eq
	ret
endfunc plat_is_my_cpu_primary

	/* -----------------------------------------------------
	 * void plat_secondary_cold_boot_setup (void);
	 *
	 * This function performs any platform specific actions
	 * needed for a secondary cpu after a cold reset. Right
	 * now this is a stub function.
	 * -----------------------------------------------------
	 */
func plat_secondary_cold_boot_setup
	bl  plat_my_core_pos
	mov x1, #1
	lsl x3, x1, x0     
	mov	x0, #0
	mov_imm	x0, PLAT_QSERVER_TRUSTED_MAILBOX_BASE

	/* Wait until the mailbox gets populated */
	mov x2, #0
poll_mailbox:
        ldr x1, [x0, #8]
        mov x5, x1
        and x1, x1, x3
	cbz	x1, 1f
	ldr	x1, [x0]
	cbz	x1, 1f
	br	x1
1:
	add x2, x2, #1
        str     x2, [x0, #0x10]
	b	poll_mailbox
	ret
endfunc plat_secondary_cold_boot_setup
	/* -----------------------------------------------------
	 * void platform_mem_init(void);
	 *
	 * We don't need to carry out any memory initialization
	 * on HIKEY. The Secure RAM is accessible straight away.
	 * -----------------------------------------------------
	 */
func platform_mem_init
	ret
endfunc platform_mem_init

	/* ---------------------------------------------
	 * int plat_crash_console_init(void)
	 * Function to initialize the crash console
	 * without a C Runtime to print crash report.
	 * Clobber list : x0, x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_init
	mov_imm	x0, CRASH_CONSOLE_BASE
	mov_imm	x1, PL011_UART_CLK_IN_HZ
	mov_imm	x2, PL011_BAUDRATE
	b	console_pl011_core_init
endfunc plat_crash_console_init

	/* ---------------------------------------------
	 * int plat_crash_console_putc(int c)
	 * Function to print a character on the crash
	 * console without a C Runtime.
	 * Clobber list : x1, x2
	 * ---------------------------------------------
	 */
func plat_crash_console_putc
	mov_imm	x1, CRASH_CONSOLE_BASE
	b	console_pl011_core_putc
endfunc plat_crash_console_putc
	/* ---------------------------------------------
	 * int plat_crash_console_flush()
	 * Function to force a write of all buffered
	 * data that hasn't been output.
	 * Out : return -1 on error else return 0.
	 * Clobber list : x0, x1
	 * ---------------------------------------------
	 */
func plat_crash_console_flush
	mov_imm	x0, CRASH_CONSOLE_BASE
	b	console_pl011_core_flush
endfunc plat_crash_console_flush
	/* ---------------------------------------------
	 * void plat_report_exception(unsigned int type)
	 * Function to report an unhandled exception
	 * with platform-specific means.
	 * On HIKEY platform, it updates the LEDs
	 * to indicate where we are
	 * ---------------------------------------------
	 */
func plat_report_exception
	mov	x8, x30

	mrs	x2, currentel
	and	x2, x2, #0xc
	/* Check EL1 */
	cmp	x2, #0x04
	beq	plat_report_el1

	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_el3_str
	bl	asm_print_str

	mrs	x4, esr_el3
	bl	asm_print_hex

	adr	x4, elr_el3_str
	bl	asm_print_str

	mrs	x4, elr_el3
	bl	asm_print_hex
	b	plat_report_end

plat_report_el1:
	adr	x4, plat_err_str
	bl	asm_print_str

	adr	x4, esr_el1_str
	bl	asm_print_str

	mrs	x4, esr_el1
	bl	asm_print_hex

	adr	x4, elr_el1_str
	bl	asm_print_str

	mrs	x4, elr_el1
	bl	asm_print_hex

        adr	x4, far_el1_str
	bl	asm_print_str

	mrs	x4, far_el1
	bl	asm_print_hex
plat_report_end:
        adr	x4, lr_str
	bl	asm_print_str

	mov	x4, x8
	bl	asm_print_hex
	mov	x30, x8
	ret
endfunc plat_report_exception

	/* -----------------------------------------------------
	 * void plat_reset_handler(void);
	 * -----------------------------------------------------
	 */
func plat_reset_handler
	ret
endfunc plat_reset_handler

.section .rodata.rev_err_str, "aS"
plat_err_str:
	.asciz "\nPlatform exception reporting:"
esr_el3_str:
	.asciz "\nESR_EL3: "
elr_el3_str:
	.asciz "\nELR_EL3: "
esr_el1_str:
	.asciz "\nESR_EL1: "
elr_el1_str:
	.asciz "\nELR_EL1: "
far_el1_str:
	.asciz "\nFAR_EL1: "
lr_str:
	.asciz "\nLR: "
secondary_core_str:
	.asciz "\n2nd core mpidr: "
secondary_pos_str:
	.asciz "\n2nd core pos: "
secondary_entry_str:
	.asciz "\n2nd entry: "
