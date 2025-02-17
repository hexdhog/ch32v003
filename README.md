# Bare blinky

## The goal

I have messed with electronics for quite some time now, pretty much ever since I started programming. Actually, I learned basic C programming by playing in Arduino IDE. So I have a decent understanding of how to write basic programs that run on Arduino-like compatible microcontrollers; but I have a mediocre understanding of what is actually going on when I use the Arduino or the chip's framework.

So I want to go down the software stack and understand what really happens. Making an LED blink, an extremely basic task, in assembly seems like a proper way to start.

## The setup

I bought a [CH32V003 kit](https://es.aliexpress.com/item/1005005834050641.html) a while back ago with this goal in mind.

It's a microcontroller based on QuingKe RISC-V2A, 2KB SRAM, 16KB FLASH, PFIC and comes with a bunch of very common peripherals (e.g. I2C, USART, SPI, ADC, etc). The kit also includes a WCH-LinkE, a USB to SWIO bridge, which is used to program the microcontroller and monitor the USART interface.

I will be using PlatformIO to manage the project, although that isn't really relevant to this post.

## The plan

### What do we need?

The CH32V003 kit's PCB has an LED connected to GPIO D4 which emits light when that pin is low. So we need a way to manipulate the GPIO pin. Then we need a way to time the actions perfomed on the GPIO pin, so that the LED actually blinks and it does so at a constant rate.

The following block diagram depicts the CH32V003 system architecture:

![CH32V003 system architecture block diagram](./img/ch32v003-block-diagram.png)

There are three GPIO ports: GPIOA (PA1-PA2), GPIOC (PC0-PC7) and GPIOD (PD0-PD7) which are completely seperate GPIO controllers. Like most peripherals, they are accessed through the [AHB bus](https://en.wikipedia.org/wiki/Advanced_Microcontroller_Bus_Architecture). Each of these peripheral controllers has a set of registers that control the behaviour of the actual peripheral, and each of these registers is wired to the bus. Additionally, although not specified in the block diagram, the CH32V003 core has a system tick counter which we can use to time the GPIO actions. Like the peripherals on the AHB bus, it has a set of registers that control its behaviour.

This is what's called [Memory Mapped IO](https://en.wikipedia.org/wiki/Memory-mapped_I/O_and_port-mapped_I/O), and it is very common in microcontrollers. The registers from the peripheral controllers are each assigned a unique memory address. Therefore, interfacing with each register translates to a read and/or write operation from/to the bus. The CH32V003 has the following memory map:

![CH32V003 memory map](./img/ch32v003-memory-map.png)

Note that FLASH and SRAM are also peripherals and, while they are connected on different buses, are accesed in the same way as any other peripherals. This means that all components are wired in a way such that when the RISC-V2A core selects an address to read/write to, the correct bus and peripheral controller register is enabled and given access to the bus. So to us, the RISC-V2A core programmers, accessing any peripheral analogous to reading/writing data to memory.

According to the memory map diagram, the GPIO port D registers are located between addresses `0x40011400` and `0x40011800`, and, though not specified, the system tick timer's registers are located in the Core Private Peripherals section (`0xe000000` to `0xe0100000`).

### System clock setup

Before setting anything else up we should initialize the system clock. After reset, the CH32V003 uses the HSI (High Speed Internal) oscillator at 24MHz as a clock source. The [PLL (Phase Locked Loop)](https://en.wikipedia.org/wiki/Phase-locked_loop), used to multiply the input clock source, is disabled. An HSE (High Speed Extenal) oscillator 4-25MHz can also be used as a clock source which is disabled after reset; up to de user to set it up on every startup.

The following diagram shows the system clock tree block diagram:

![CH32V003 clock tree block diagram](./img/ch32v003-clock-tree.png)

A few important notes about the clock tree:
- SYSCLK output is multiplexed between HSI, HSE, HSI\*2 and HSE\*2
- AHB clock source can be prescaled (dividable by: 1, 2, ..., 256)
- Core System Timer can be divided by 8.

Before doing anything else the SYSCLK must be configured. So let's configure it at 48MHz, as it is the maximum supported frequency. The steps needed to do this are:

1. Enable HSI and PLL
2. Turn off prescaler (do not divide AHB clock) and select HSI as PLL source
3. Configure flash to use 1 cycle latency (recommended when 24MHz <= SYSCLK <= 48MHz)
4. Clear RCC interrupt flags
5. Wait until PLL is ready
6. Select PLL as system clock
7. Wait until PLL is used as system clock

All of this can be done through the RCC (Reset and Clock Control) registers, with base address `0x40021000`:

![CH32V003 RCC registers](./img/ch32v003-rcc-registers.png)

and the FLASH registers, with base address `0x40022000`:

![CH32V003 FLASH registers](./img/ch32v003-flash-registers.png)

More specifically, we'll need the following registers:

**R32_RCC_CTLR**
![CH32V003 R32_RCC_CTLR](./img/ch32v003-rcc-ctlr.png)

**R32_RCC_CFGR0**
![CH32V003 R32_RCC_CFGR0](./img/ch32v003-rcc-cfgr0.png)

**R32_RCC_INTR**
![CH32V003 R32_RCC_INTR](./img/ch32v003-rcc-intr.png)

**R32_FLASH_ACTLR**
![CH32V003 R32_FLASH_INTR](./img/ch32v003-flash-actlr.png)

**Note**: the description of each field for all registers is left out for brevity. More information can be found in the reference manual.

First, HSI and PLL are enabled through `R32_RCC_CTLR` field `HSION` (bit 0) and field `PLLON` (bit 24). For both fields, writing a 1 will enable the device and writing a 0 will disable it. So let's write some RISC-V assembly code to do exactly this:

```riscv
.equ rcc_base, 0x40021000
.equ flash_r_base, 0x40022000
.equ gpio_pd_base, 0x40011400
.equ systck_base, 0xe000f000

.globl main
main:
        li a0, rcc_base # a0 -> RCC register base address
        li a1, flash_r_base # a1 -> FLASH register base address
        li a2, gpio_pd_base # a2 -> GPIO port d register base address

        # PLL_ON (bit 0): enable PLL
        # HSI_ON (bit 24): enable HSI
        #     RCC CTLR = 1 << 0 | 1 << 24
        #     RCC CTLR = 0x01000001
        li t0, 0x01000001
        sw t0, 0(a0)
```

Second, the prescaler is turned off by writing 0 to `R32_RCC_CFGR0` field `HPRE` (bits 4-7) and HSI is selected as PLL source by writing 0 to field `PLLSRC` (bit 16):

```riscv
        # HPRE = 0: prescaler off; do not divide SYSCLK
        # PLLSRC = 0: HSI (instead of HSE) for PLL input
        #     RCC_CFGR0 = 0 << 4 | 0 << 16
        #     RCC_CFGR0 = 0
        li t0, 0x00000000
        sw t0, 4(a0)
```

Third, flash latency is configured through `R32_FLASH_ACTLR` field `LATENCY` (bits 0-1); writing a 1 will select a 1 cycle latency:

```riscv
        # configure flash to recommended settings for 48MHz clock
        # LATENCY (bits 0-1) = 1
        #     FLASH_ACTLR = 1 << 0
        #     FLASH_ACTLR = 1
        li t0, 0x00000001
        sw t0, 0(a1)
```

The fourth step, clearing the RCC interrupt flags, involves multiple fields of `R32_RCC_INTR`, so let's take a closer look at each of these fields to better understand how this register works:

| bit | name       | access | desc                                               |
|-----|------------|--------|----------------------------------------------------|
| 0   | `LSIRDYF`  | RO     | LSI clock-ready interrupt flag                     |
| 2   | `HSIRDYF`  | RO     | HSI clock-ready interrupt flag                     |
| 3   | `HSERDYF`  | RO     | HSE clock-ready interrupt flag                     |
| 4   | `PLLRDYF`  | RO     | PLL clock-ready lockout interrupt flag             |
| 7   | `CSSF`     | RO     | Clock security system interrupt flag bit           |
| 8   | `LSIRDYIE` | RW     | LSI-ready interrupt enable bit                     |
| 10  | `HSIRDYIE` | RW     | HSI-ready interrupt enable bit                     |
| 11  | `HSERDYIE` | RW     | HSE-ready interrupt enable bit                     |
| 12  | `PLLRDYIE` | RW     | PLL-ready interrupt enable bit                     |
| 16  | `LSIRDYC`  | WO     | Clear the LSI oscillator ready interrupt flag bit  |
| 19  | `HSERDYC`  | WO     | Clear the HSE oscillator ready interrupt flag bit  |
| 18  | `HSIRDYC`  | WO     | Clear the HSI oscillator ready interrupt flag bit  |
| 20  | `PLLRDYC`  | WO     | Clear the PLL-ready interrupt flag bit             |
| 23  | `CSSC`     | WO     | Clear the clock security system interrupt flag bit |

The first 5 table entries are interrupt flags, indicated by the trailing `F`, and are read-only because they are set by hardware. The last 5 table entries are fields only used to clear the interrupt flags, indicated by the trailing `C`, and are write-only. Because the interrupt flags are set by hardware, these fields are need to physically "reset" the corresponding hardware, which will in turn clear the corresponding interrupt flag. Finally, the middle 4 table entries are interrupt enable fields, indicated by the trailing `IE`. When set to 1 the an interrupt will be generated when the corresponding interrupt flag is set.

For our use case, we actually only *need* to clear certain interrupt flags for in order to know when certain events happend (e.g. we'll need to know when HSI and PLL are ready after we have enabled them), but clearing all of the interrupt flags is a good idea when changing the clock tree configuration anyway, so we'll do that. Also, we could write our program in a way that doesn't actively wait for the peripherals to be ready by utilizing interrupts but that would complicate our code, so we'll disable interrupt too:

```riscv
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
```

```riscv
```

```riscv
```

```riscv
```

```riscv
```

```riscv
```

### GPIO port setup

### System tick counter as timer

### Blinky
