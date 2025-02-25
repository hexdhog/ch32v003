typedef unsigned int uint32_t;

typedef struct {
	volatile uint32_t CTLR;			// clock control register
	volatile uint32_t CFGR0;		// clock configuration register 0
	volatile uint32_t INTR;			// clock interrupt register
	volatile uint32_t APB2PRSTR;	// PB2 peripheral reset register
	volatile uint32_t APB1PRSTR;	// PB1 peripheral reset register
	volatile uint32_t AHBPCENR;		// HB peripheral clock enable register
	volatile uint32_t APB2PCENR;	// PB2 peripheral clock enable register
	volatile uint32_t APB1PCENR;	// PB1 clock enable register
	uint32_t RESERVED;
	volatile uint32_t RSTSCK;		// control/status register
} rcc_reg_t;

typedef struct {
	volatile uint32_t CFGLR;	// port configuration register low
	volatile uint32_t CFGHR;	// port configuration register high (not needed in ch32v003 as pin count < 8 for any port)
	volatile uint32_t INDR;		// port input data register
	volatile uint32_t OUTDR;	// port output data register
	volatile uint32_t BSHR;		// port set/reset register
	volatile uint32_t BCR;		// port reset register
	volatile uint32_t LCKR;		// port configuration lock register
} gpio_reg_t;

typedef struct {
	volatile uint32_t ACTLR;			// control register
	volatile uint32_t KEYR;				// FPEC key register
	volatile uint32_t OBKEYR;			// OBKEY register
	volatile uint32_t STATR;			// status register
	volatile uint32_t CTLR;				// configuration register
	volatile uint32_t ADDR;				// address register
	uint32_t RESERVED;
	volatile uint32_t OBR;				// option byte register
	volatile uint32_t WPR;				// write protection register
	volatile uint32_t MODEKEYR;			// extended key register
	volatile uint32_t BOOT_MODEKEYR;	// unlock boot key register
} flash_reg_t;

typedef struct {
	volatile uint32_t CTLR;		// system count control register
	volatile uint32_t SR;		// system count status register
	volatile uint32_t CNT;		// system counter register
	uint32_t RESERVED0;
	volatile uint32_t CMP;	// counting comparison register
	uint32_t RESERVED1;
} systick_reg_t;

// #define FLASH_BASE      ((uint32_t) 0x08000000)
// #define SRAM_BASE       ((uint32_t) 0x20000000)
// #define PERIPH_BASE     ((uint32_t) 0x40000000)

#define GPIO_PA_BASE	((uint32_t) 0x40010800)
#define GPIO_PC_BASE	((uint32_t) 0x40011000)
#define GPIO_PD_BASE	((uint32_t) 0x40011400)

#define RCC_BASE		((uint32_t) 0x40021000)

#define FLASH_R_BASE	((uint32_t) 0x40022000)

#define SYSTCK_BASE	((uint32_t) 0xe000f000)

#define RCC				((rcc_reg_t *) RCC_BASE)
#define GPIOA			((gpio_reg_t *) GPIO_PA_BASE)
#define GPIOC			((gpio_reg_t *) GPIO_PC_BASE)
#define GPIOD			((gpio_reg_t *) GPIO_PD_BASE)
#define FLASH			((flash_reg_t *) FLASH_R_BASE)
#define SYSTCK			((systick_reg_t *) SYSTCK_BASE)

#define RCC_CTLR_PLLON			((uint32_t) 0x01000000)
#define RCC_CTLR_PLLRDY			((uint32_t) 0x02000000)
#define RCC_CTLR_HSION			((uint32_t) 0x00000001)

#define RCC_CFGR0_SW_PLL		((uint32_t) 0x00000002)
#define RCC_CFGR0_SW			((uint32_t) 0x00000003)

#define RCC_CFGR0_SWS_PLL		((uint32_t) 0x00000008)
#define RCC_CFGR0_SWS			((uint32_t) 0x0000000C)

#define RCC_CFGR0_HBPRE_OFF		((uint32_t) 0x00000000)

#define RCC_CFGR0_PLLSRC_HSI	((uint32_t) 0x00000000)

#define RCC_APB2PCENR_IOPDEN	((uint32_t) 0x00000020)

#define FLASH_CTLR_LATENCY_1    ((uint32_t) 0x00000001)


#define GPIO_CFGLR_MODE_10MHz	((uint32_t) 0x00000001)
#define GPIO_CFGLR_CNF_OUT_PP	((uint32_t) 0x00000000)


#define LED_PIN 4

void delay_systick(uint32_t n) {
	SYSTCK->CTLR &= ~((uint32_t) 1); // turn off system counter
	SYSTCK->SR &= ~((uint32_t) 1); // clear count value comparison flag
	SYSTCK->CMP = n; // count end value
	SYSTCK->CNT = 0; // count start value
	SYSTCK->CTLR |= 1; // turn on system counter
	while (!(SYSTCK->SR & 1));
	SYSTCK->CTLR &= ~((uint32_t) 1); // turn off system counter
}

void delay_ms(uint32_t n) {
	delay_systick(n * (48000000/8000));
}

int main() {
	// setup clock to 48MHz
	RCC->CTLR = RCC_CTLR_PLLON | RCC_CTLR_HSION; // use HSI and enable PLL
	RCC->CFGR0 = RCC_CFGR0_HBPRE_OFF | RCC_CFGR0_PLLSRC_HSI;	// prescaler off -> do not divide SYSCLK
																// select HSI (instead of HSE) for PLL input
	FLASH->ACTLR = FLASH_CTLR_LATENCY_1; // 1 cycle latency (recommended in the documentation when 24MHz <= SYSCLK <= 48MHz)
	RCC->INTR = 0x009f0000;
	// 0x009f0000 -> 0b 0000 0000 1001 1101 0000 0000 0000 0000
	//                  CSSC = 1 -> clear CSSF (clock security system interrupt flag bit)
	//                  PLLRDYC = 1 -> clear PLLRDYF (PLL-ready interrupt flag bit)
	//                  HSERDYC = 1 -> clear HSERDYF (HSE oscillator ready interrupt flag bit)
	//                  HSIRDYC = 1 -> clear HSIRDYF (HSI oscillator ready interrupt flag bit)
	//                  LSIRDYC = 1 -> clear LSIRDYF (LSI oscillator ready interrupt flag bit)

	while ((RCC->CTLR & RCC_CTLR_PLLRDY) == 0); // wait until PLL is ready
	RCC->CFGR0 = (RCC->CFGR0 & ~RCC_CFGR0_SW) | RCC_CFGR0_SW_PLL; // select PLL as system clock
	while ((RCC->CFGR0 & RCC_CFGR0_SWS) != RCC_CFGR0_SWS_PLL); // wait until PLL is used as system clock

	// setup GPIO pin for led
	RCC->APB2PCENR |= RCC_APB2PCENR_IOPDEN; // enable GPIO port D

	GPIOD->CFGLR &= ~(0x0f << (4 * LED_PIN)); // clear mode and configuration fields for selected pin
	GPIOD->CFGLR |= (GPIO_CFGLR_CNF_OUT_PP | GPIO_CFGLR_MODE_10MHz) << (4 * LED_PIN);

	// uint32_t mask = 1 << LED_PIN;
	while (1) {
		GPIOD->BSHR = 1 << LED_PIN;
		delay_ms(1000);
		GPIOD->BSHR = 1 << (LED_PIN + 16);
		delay_ms(100);
	}
}
