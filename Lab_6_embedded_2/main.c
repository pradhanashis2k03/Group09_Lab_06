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
int cnt =0;

void SysTick_Init(void) {
    STCTRL = 0;                   // Disable SysTick during setup
    STRELOAD = 0x00FFFFFF;         // Max reload value
    STCURRENT = 0;                 // Clear current value
    STCTRL |= (CLKINT | ENABLE);   // Use system clock and enable SysTick
}

uint32_t getSysTickValue(void) {
    return STCURRENT;  // Return the current value of SysTick
}

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

volatile uint32_t pressTime = 0;    // Variable to store press start time
volatile uint8_t buttonPressed = 0; // State to track button press

void GPIOPortF_Handler(void) {
    if (GPIO_PORTF_RIS_R & (1 << 4)) {  // Check if interrupt occurred on PF4 (Switch 1)
        GPIO_PORTF_ICR_R |= (1 << 4);   // Clear the interrupt flag for PF4

        if ((GPIO_PORTF_DATA_R & (1 << 4)) == 0) {  // Switch is pressed (PF4 is low)
            if (!buttonPressed) {
                buttonPressed = 1;               // Mark button as pressed
                pressTime = getSysTickValue();   // Record press start time
            }
        } else {  // Switch is released (PF4 is high)
            if (buttonPressed) {
                buttonPressed = 0;  // Mark button as released
                uint32_t releaseTime = getSysTickValue();
                uint32_t duration = (pressTime - releaseTime) & 0x00FFFFFF;  // Calculate press duration
                pressTime = 0;

                // Convert SysTick duration to milliseconds
                uint32_t timeInMs = duration / (CLOCK_MHZ * 1000);

                // Adjust duty cycle based on press duration
                if (timeInMs >= 1000) {  // Pressed for more than 1 second
                    if (d > 0) {
                        d -= 10;  // Decrease duty cycle
                    }
                } else {  // Pressed for less than 1 second
                    if (d < 100) {
                        d += 10;  // Increase duty cycle
                    }
                }
            }
        }
    }
}


int main(void) {
    // Enable clock for GPIO Port F
    SYSCTL_RCGCGPIO_R |= (1 << 5); // Enable clock for Port F
    while ((SYSCTL_PRGPIO_R & (1 << 5)) == 0);  // Wait until Port F is ready

    // Unlock PF4 (switch)
    GPIO_PORTF_LOCK_R = 0x4C4F434B;  // Unlock GPIO Port F
    GPIO_PORTF_CR_R |= 0x10;     // Allow changes to PF4

    // Configure PF4 (Switch) as input and PF1 (LED) as output
    GPIO_PORTF_DIR_R &= ~(0x10);   // Set PF4 as input (switch)
    GPIO_PORTF_DIR_R |= 0x02;    // Set PF1 as output (LED)
    GPIO_PORTF_DEN_R |= 0x13; // Enable digital for PF4 and PF1
    GPIO_PORTF_PUR_R |= 0x10;    // Enable pull-up on PF4

    // Disable interrupt for PF4 during configuration
    GPIO_PORTF_IM_R &= ~(0x10);

    // Set PF4 to trigger interrupt on falling edge (button press)
    GPIO_PORTF_IS_R |= 0x10;
    GPIO_PORTF_IEV_R &= ~(0x10);

    // Clear any prior interrupt on PF4
    GPIO_PORTF_ICR_R |= 0x10;

    // Enable interrupt on PF4
    GPIO_PORTF_IM_R |= 0x10;

    // Enable the interrupt in NVIC (IRQ30 for GPIO Port F)
    NVIC_EN0_R |= (1 << 30);

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
