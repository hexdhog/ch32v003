.section .init
.globl start
start:
        # there isn't really a need for setting up gp and sp since they are not used in this program
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

.equ ms_to_tick, 48000000/8000

.equ rcc_base, 0x40021000
.equ flash_r_base, 0x40022000
.equ gpio_pd_base, 0x40011400
.equ systck_base, 0xe000f000

.equ led_pin, 4

.globl main
main:
        # setup clock to 48MHz
        li a0, rcc_base # a0 -> RCC register base address
        li a1, flash_r_base # a1 -> FLASH register base address
        li a2, gpio_pd_base # a2 -> GPIO port d register base address

        # PLL_ON (bit 0): enable PLL
        # HSI_ON (bit 24): enable HSI
        #     RCC_CTLR = 1 << 0 | 1 << 24
        #     RCC_CTLR = 0x01000001
        li t0, 0x01000001
        sw t0, 0(a0)

        # HPRE = 0: prescaler off; do not divide SYSCLK
        # PLLSRC = 0: HSI (instead of HSE) for PLL input
        #     RCC_CFGR0 = 0 << 4 | 0 << 16
        #     RCC_CFGR0 = 0
        li t0, 0x00000000
        sw t0, 4(a0)

        # configure flash to recommended settings for 48MHz clock
        # LATENCY (bits 0-1) = 1
        #     FLASH_ACTLR = 1 << 0
        #     FLASH_ACTLR = 1
        li t0, 0x00000001
        sw t0, 0(a1)

        # CSSC     (bit 23) = 1 -> clear CSSF (clock security system interrupt flag bit)
        # PLLRDYC  (bit 20) = 1 -> clear PLLRDYF (PLL-ready interrupt flag bit)
        # HSERDYC  (bit 19) = 1 -> clear HSERDYF (HSE oscillator ready interrupt flag bit)
        # HSIRDYC  (bit 18) = 1 -> clear HSIRDYF (HSI oscillator ready interrupt flag bit)
        # LSIRDYC  (bit 16) = 1 -> clear LSIRDYF (LSI oscillator ready interrupt flag bit)
        # PLLRDYIE (bit 12) = 0 -> disable PLL-ready interrupt
        # HSERDYIE (bit 11) = 0 -> disable HSE-ready interrupt
        # HSIRDYIE (bit 10) = 0 -> disable HSI-ready interrupt
        # LSIRDYIE (bit  8) = 0 -> disable LSI-ready interrupt
        #     RCC_INTR = 1<<23 | 1<<20 | 1<<19 | 1<<18 | 1<<16 | 0<<12 | 0<<11 | 0<<10 | 0<<8
        #     RCC_INTR = 0b 0000 0000 1001 1101 0000 0000 0000 0000
        #     RCC_INTR = 0x009d0000
        li t0, 0x009d0000
        sw t0, 8(a0)

        # wait until PLL is ready
        li t1, 0x02000000 # PLL_RDY mask
.L_pll_rdy_wait:
        lw t0, 0(a0) # RCC CTLR
        and t0, t0, t1
        beq t0, zero, .L_pll_rdy_wait

        # RCC_CFGR0 = RCC_CFGR0 & ~(0b11) | 0b10
        # RCC_CFGR0 = RCC_CFGR0 & ~(0x00000003) | 0x00000002
        # RCC_CFGR0 = RCC_CFGR0 & 0xfffffffc | 0x00000002
        lw t0, 4(a0) # t0 = RCC CFGR0
        and t0, t0, 0xfffffffc # ~(RCC CFGR0 SW) = ~(0x00000003) = 0xfffffffc
        or t0, t0, 0x00000002 # RCC CFGR0 SW PLL = 0x00000002
        sw t0, 4(a0)

        # wait until PLL is used as SYSCLK
        li t1, 0x0000000c # RCC CFGR0 SWS mask
        li t2, 0x00000008 # RCC CFGR0 SW PLL
.L_pll_use_wait:
        # RCC CFGR0
        lw t0, 4(a0)
        and t0, t0, t1
        bne t0, t2, .L_pll_use_wait

        # setup GPIO pin for led
        # enable GPIO port D
        lw t0, 24(a0) # t0 = APB2PCENR
        or t0, t0, 0x00000020 # APB2PCENR | EPB2PCENR_IOPDEN
        sw t0, 24(a0)

        # GPIOD->CFGLR &= ~(0x0f << (4 * LED_PIN)); // clear mode and configuration fields for selected pin
        lw t0, 0(a2)
        li t1, ~(0x0f << (4 * led_pin))
        and t0, t0, t1

	# GPIOD->CFGLR |= (GPIO_CFGLR_CNF_OUT_PP | GPIO_CFGLR_MODE_10MHz) << (4 * LED_PIN);
        li t1, 0x00000001 << (4 * led_pin)
        or t0, t0, t1
        sw t0, 0(a2)

        li t2, 1 << led_pin # pin mask
.L_loop:
        sw t2, 20(a2)
        li a0, 100*ms_to_tick
        call delay_systick

        sw t2, 16(a2)
        li a0, 1000*ms_to_tick
        call delay_systick

        j .L_loop

delay_systick:
# SYSTCK->SR &= ~((uint32_t) 1); // clear count value comparison flag
# SYSTCK->CMP = n; // count end value
# SYSTCK->CNT = 0; // count start value
# SYSTCK->CTLR |= 1; // turn on system counter
# while (!(SYSTCK->SR & 1));
# SYSTCK->CTLR &= ~((uint32_t) 1); // turn off system counter

        li a1, systck_base # a1 -> system tick register base address

        # stop system counter
        lw t0, 0(a1)
        and t0, t0, 0xfffffffe # set last bit (STE) to 0
        sw t0, 0(a1)

        # clear count value comparison flag
        li t0, 0xfffffffe # t0 = ~(1)
        sw t0, 4(a1)

        # set count start value
        sw zero, 8(a1)
        
        # set count end value
        sw a0, 16(a1)

        # start system counter
        lw t0, 0(a1)
        or t0, t0, 0x00000001 # set last bit (STE) to 1
        sw t0, 0(a1)

        # wait until count system counter has reached target number
.L_wait:
        lw t0, 4(a1)
        and t0, t0, 0x00000001
        beq t0, zero, .L_wait

        # stop system counter
        lw t0, 0(a1)
        and t0, t0, 0xfffffffe # set last bit (STE) to 0
        sw t0, 0(a1)

        ret

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
