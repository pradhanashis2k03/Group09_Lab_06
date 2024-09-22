#include <stdint.h>
#include <stdbool.h>
#include "tm4c123gh6pm.h"

/* SysTick memory-mapped registers */
#define STCTRL *((volatile long *) 0xE000E010) // control and status
#define STRELOAD *((volatile long *) 0xE000E014) // reload value
#define STCURRENT *((volatile long *) 0xE000E018) // current value

#define COUNT_FLAG (1 << 16) // bit 16 of CSR automatically set to 1
// when timer expires
#define ENABLE (1 << 0) // bit 0 of CSR to enable the timer
#define CLKINT (1 << 2) // bit 2 of CSR to specify CPU clock

#define CLOCK_MHZ 16

volatile int d = 50;

void Delay(int ms)
{
STRELOAD = CLOCK_MHZ*10*ms; // reload value for 'ms' milliseconds
STCTRL |= (CLKINT | ENABLE); // set internal clock, enable the timer

while ((STCTRL & COUNT_FLAG) == 0) // wait until flag is set
{
; // do nothing
}
STCTRL = 0; // stop the timer
GPIO_PORTF_DATA_R ^= 0x02;

return;
}

// Interrupt handler for GPIO Port F
void GPIOPortF_Handler(void) {
    // Check if interrupt occurred on PF4 (Switch)
    if (GPIO_PORTF_RIS_R & (1 << 4)) {
        GPIO_PORTF_ICR_R |= (1 << 4);  // Clear the interrupt flag
        if (d<100){
            d += 5;  //Increase duty cycle
        }
    }
    if (GPIO_PORTF_RIS_R & (1 << 0)) {
        GPIO_PORTF_ICR_R |= (1 << 0);  // Clear the interrupt flag for PF0
        if (d > 0) {
            d -= 5;  // Decrease duty cycle
        }
    }
}

int main(void) {
    // Enable clock for GPIO Port F
    SYSCTL_RCGCGPIO_R |= (1 << 5); // Enable clock for Port F
    while ((SYSCTL_PRGPIO_R & (1 << 5)) == 0);  // Wait until Port F is ready

    // Unlock PF4 (switch)
    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // Unlock GPIO Port F
    GPIO_PORTF_CR_R |= 0x11;     // Allow changes to PF4

    // Configure PF4 (Switch) as input and PF1 (LED) as output
    GPIO_PORTF_DIR_R &= ~(0x11);   // Set PF4 as input (switch)
    GPIO_PORTF_DIR_R |= 0x02;    // Set PF1 as output (LED)
    GPIO_PORTF_DEN_R |= 0x13; // Enable digital for PF4 and PF1
    GPIO_PORTF_PUR_R |= 0x11;    // Enable pull-up on PF4

    // Disable interrupt for PF4 during configuration
    GPIO_PORTF_IM_R &= ~(0x11);

    // Set PF4 to trigger interrupt on falling edge (button press)
    GPIO_PORTF_IS_R &= ~(0x11);    // Edge-sensitive
    GPIO_PORTF_IBE_R &= ~(0x11);   // Single edge
    GPIO_PORTF_IEV_R &= ~(0x11);   // Falling edge

    // Clear any prior interrupt on PF4
    GPIO_PORTF_ICR_R |= 0x11;

    // Enable interrupt on PF4
    GPIO_PORTF_IM_R |= 0x11;

    // Enable the interrupt in NVIC (IRQ30 for GPIO Port F)
    NVIC_EN0_R |= (1 << 30);

//    NVIC_PRI7_R = (NVIC_PRI7_R & 0xFF00FFFF) | (2 << 21);

    // Enable global interrupts
    __asm("     CPSIE I");

    // Initial state: turn off the LED
    GPIO_PORTF_DATA_R &= ~(1 << 1);

    while(1) {
        if (d==0){
            GPIO_PORTF_DATA_R &= ~(0x02);
        }else if (d==100){
            GPIO_PORTF_DATA_R |= 0x02;
        }else{
            GPIO_PORTF_DATA_R &= ~(0x02);
            Delay(d);
            Delay(100-d);
        }
    }
}
