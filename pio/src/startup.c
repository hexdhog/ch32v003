typedef unsigned int uint32_t;

int main() __attribute__((used));
void start() __attribute__((naked)) __attribute__((section(".init"))) __attribute__((used));
void isr_default() __attribute__((naked)) __attribute__((section(".text.vector_handler"))) __attribute__((used));

void isr_default() {
	asm volatile ("1: j 1b");
}

void isr_nmi() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_hard_fault() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_sys_tick() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_sw() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_wwdg() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_pvd() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_flash() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_rcc() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_exti() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_wake_up() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch2() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch3() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch4() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch5() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch6() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_dma_ch7() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_adc() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_i2c_ev() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_i2c_err() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_usart_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_spi_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_tim_brk_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_tim_up_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_tim_trg_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_cc_1() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));
void isr_2() __attribute__((section(".text.vector_handler"))) __attribute__((weak,alias("isr_default"))) __attribute__((used));

void start() {
	asm volatile("\
		.option push\n\
		.option norelax\n\
		la gp, __global_pointer$\n\
		.option pop\n\
		la sp, __stack_end\n"
		// Setup the interrupt vector, processor status and INTSYSCR.
	"	li t0, 0x80\n\
		csrw mstatus, t0\n\
		li t0, 0x3\n\
		csrw 0x804, t0\n\
		la t0, isr_vector\n\
		ori t0, t0, 3\n\
		csrw mtvec, t0\n");

	asm volatile("csrw mepc, %0" : : "r"((uint32_t) main));
	asm volatile("mret\n");

	asm volatile (".align 2\n\
		.option norvc;\n\
	isr_vector:\n\
		.word  start\n\
		.word  0\n\
		.word  isr_nmi\n\
		.word  isr_hard_fault\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  0\n\
		.word  isr_sys_tick\n\
		.word  0\n\
		.word  isr_sw\n\
		.word  0\n\
		.word  isr_wwdg\n\
		.word  isr_pvd\n\
		.word  isr_flash\n\
		.word  isr_rcc\n\
		.word  isr_exti\n\
		.word  isr_wake_up\n\
		.word  isr_dma_ch1\n\
		.word  isr_dma_ch2\n\
		.word  isr_dma_ch3\n\
		.word  isr_dma_ch4\n\
		.word  isr_dma_ch5\n\
		.word  isr_dma_ch6\n\
		.word  isr_dma_ch7\n\
		.word  isr_adc\n\
		.word  isr_i2c_ev\n\
		.word  isr_i2c_err\n\
		.word  isr_usart_1\n\
		.word  isr_spi_1\n\
		.word  isr_tim_brk_1\n\
		.word  isr_tim_up_1\n\
		.word  isr_tim_trg_1\n\
		.word  isr_cc_1\n\
		.word  isr_2\n\
	");
}
