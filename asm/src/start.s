.section .init
.globl start
start:
        # there isn't really a need for setting up gp and sp since they are not
        # used in this program
.option push
.option norelax
        la gp, __global_pointer$
.option pop
        la sp, __stack_end

        # set CSR register MSTATUS (Machine Status)
        #     bit 7: MPIE (Machine Previous Interrupt Enable) to 1, which will enable interrupts when mret is executed
        #     bit 3: MIE (Machine Interrupt Enable) to 0, which disables interrupts
        li t0, 0x80
        csrw mstatus, t0

        # set CSR register INTSYSCR (Interrupt System Control Register) located at CSR address 0x804
        #     bit 1: interrupt nesting table enable to 1
        #     bit 0: hardware stack enable to 1
        li t0, 0x3
        csrw 0x804, t0

        # set CSR register MTVEC (Exception Entry Base Address Register)
        #     bits [31:2]: interrupt vector table base address (aligned to 4 bytes; last two bits are hardwired to 0)
        #     bit 1: indentify pattern -> 1 : by absolute address
        #     bit 0: entry address -> 1 : address offset based on interrupt number*4
        la t0, isr_vector
        ori t0, t0, 3
        csrw mtvec, t0

        # set CSR register MEPC (Machine Exception Program Counter); return address of an exception handler
        la t0, main
        csrw mepc, t0
        mret
        # mret -> Machine Return (return from exception handler)
        #     1. restore MIE from MPIE
        #     2. set MPIE to 1
        #     3. jump to address stored in MEPC CSR register

.section .text

.globl main
main:
        j main

.section .text.isr_handler

.align 2
isr_default:
        j isr_default

.align 2
.option norvc
isr_vector:
        .word  start
        .word  0
        .word  isr_default
        .word  isr_default
        .word  0
        .word  0
        .word  0
        .word  0
        .word  0
        .word  0
        .word  0
        .word  0
        .word  isr_default
        .word  0
        .word  isr_default
        .word  0
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
        .word  isr_default
