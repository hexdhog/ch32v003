# Bare blinky

## The goal

I have messed with electronics for quite some time now, pretty much ever since I started programming. Actually, I learned basic C programming by playing in Arduino IDE. So I have a decent understanding of how to write basic programs that run on Arduino-like compatible microcontrollers; but I have a mediocre understanding of what is actually going on when I use the Arduino or the chip's framework.

So I want to go down the software stack and understand what really happens. Making an LED blink, an extremely basic task, in assembly seems like a proper way to start.

## The setup

I bought a [CH32V003 kit](https://es.aliexpress.com/item/1005005834050641.html) a while back ago with this goal in mind.

It's a microcontroller based on QuingKe RISC-V2A, 2KB SRAM, 16KB FLASH, PFIC and comes with a bunch of very common peripherals (e.g. I2C, USART, SPI, ADC, etc). The kit also includes a WCH-LinkE, a USB to SWIO bridge, which is used to program the microcontroller and monitor the USART interface.

I will be using PlatformIO to manage the project, although that isn't really relevant to this post.

## The plan

What does it take to make an LED blink?

Well, the CH32V003 kit's PCB has an LED connected to GPIO D4 which emits light when that pin is low. So we need a way to manipulate the GPIO pin. Then we need a way to time our actions, so that the LED actually blinks and it does so at a constant rate.

The CH32V003 has the following system architecture:

![CH32V003 block diagram](./img/ch32v003-block-diagram.png)

There are three GPIO ports: GPIOA (PA1-PA2), GPIOC (PC0-PC7) and GPIOD (PD0-PD7) which are interfaced seperately. Like most peripherals, they are connected to the [AHB bus](https://en.wikipedia.org/wiki/Advanced_Microcontroller_Bus_Architecture). Each of these peripheral controllers has a set of registers that control the behaviour of the actual peripheral. Each of these registers is wired to the bus.

This is what's called [Memory Mapped IO](https://en.wikipedia.org/wiki/Memory-mapped_I/O_and_port-mapped_I/O), and it is very common in microcontrollers. Every single register from the peripheral controllers is assigned a fixed memory address. Therefore, interfacing with each register translates to a read and/or write operation from/to the bus. The CH32V003 has the following memory map:

![CH32V003 memory map](./img/ch32v003-memory-map.png)

Note that FLASH and SRAM are also peripherals and, while they are connected on different buses, are accesed in the same way as any other peripherals. This means that all components are wired in a way such that when the RISC-V2A core selects an address to read/write to the correct bus and peripheral controller register is enabled and given access to the bus. So to us, the RISC-V2A core programmers, accessing any peripheral controller is done in the same way as accessing data on FLASH or SRAM.

According to the memory map diagram, the GPIO port D registers are located between addresses `0x40011400` and `0x40011800`.

---------






Well, we need to manipulate a GPIO pin and have some way to time the GPIO manipulations so that the LED blinks at a constant rate.

There are three GPIO ports: GPIOA (PA1-PA2), GPIOC (PC0-PC7), GPIOD (PD0-PD7), which are interfaced seperately. For the CH32V003 kit, the PCB has an LED connected to GPIO D4 which emits light when the pin is low, so this will be the pin I will use.

For the timer, the RISC-V2A core comes with a 32-bit incremental counter which we can use to count system ticks.

![CH32V003 block diagram](./img/ch32v003-block-diagram.png)

The GPIO ports are connected to the [AHB bus](https://en.wikipedia.org/wiki/Advanced_Microcontroller_Bus_Architecture) with its registers mapped to memory, so any operation is a memory read/write.

![CH32V003 memory map](./img/ch32v003-memory-map.png)

Simlarly, the system tick counter's registers are mapped to memory, but in the Core Private Peripherals section.
