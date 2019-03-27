#include <ti/devices/msp432p4xx/inc/msp.h>
#include <ti/devices/msp432p4xx/driverlib/driverlib.h>
#include <ti/grlib/grlib.h>
#include "LcdDriver/Crystalfontz128x128_ST7735.h"
#include "LcdDriver/HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.h"
//#include "HAL_I2C.h"
//#include "HAL_TMP006.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>
//#include <stdbool.h>

#ifndef MYHEADER_H_
#define MYHEADER_H_

void PWM_Init3(){
	    //![Simple Timer_A Example]
    /* Setting MCLK to REFO at 128Khz for LF mode
     * Setting SMCLK to 64Khz */
    MAP_CS_setReferenceOscillatorFrequency(CS_REFO_128KHZ);
    MAP_CS_initClockSignal(CS_MCLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);
    MAP_CS_initClockSignal(CS_SMCLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_2);
    MAP_PCM_setPowerState(PCM_AM_LF_VCORE0);

		MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN4, GPIO_PRIMARY_MODULE_FUNCTION);
}

//----------------------------------------------------------------------------------------
//SMCLK = 48MHz/4 = 12 MHz, 83.33ns
//Counter counts up to TA0CCR0 and back down
// Let Timerclock period = T = 8/12MHz = 666.7ns
//Period of P2.4 is period*1.333 us, duty cycle = duty1/period

void PWM_Init12(uint16_t period, uint16_t duty){
	P2-> DIR |= 0x30; 		//P2.4, P2.5 output
	P2->SEL0  |= 0x30;		// P2.4, P2.5 Timer0A function
	P2->SEL1  &= ~0x30;		// P2.4, P2.5 Timer0A function
	TIMER_A0 -> CCTL[0] = 0x0080;	// CCI0 toggle
	TIMER_A0 -> CCR[0]	= period;	// period is 2*period*8*83.33 ns is 1.333 * period
	TIMER_A0 -> EX0 = 0x0000;			// divide by 1
	TIMER_A0 -> CCTL[1] = 0x0040; 	//CCR1 toggle/reset
	TIMER_A0 -> CCR[1] = duty;		// CCR1 duty cycle is duty/period

	TIMER_A0 ->CTL = 0x02F0;		// SMCLK = 12 MHz, divide by 8, up-down mode
}

//----------------------------------------------------------------------------------------
void setupIO(void){
    //P2.0 RED LED on Booster Pack
    MAP_GPIO_setAsOutputPin(GPIO_PORT_P2, GPIO_PIN0);

    //P1.1 switch S1 on booster pack with interrupts
    MAP_GPIO_setAsInputPinWithPullUpResistor(GPIO_PORT_P1, GPIO_PIN1);
    MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, GPIO_PIN1);
    MAP_GPIO_enableInterrupt(GPIO_PORT_P1, GPIO_PIN1);
    MAP_Interrupt_enableInterrupt(INT_PORT1);

    //Pulse Width Modulation
    MAP_GPIO_setAsPeripheralModuleFunctionOutputPin(GPIO_PORT_P2, GPIO_PIN4,
    GPIO_PRIMARY_MODULE_FUNCTION);
}

//----------------------------------------------------------------------------------------
static int TimerA_Flag = 0;
static Timer_A_PWMConfig pwmConfig =
{
        TIMER_A_CLOCKSOURCE_SMCLK,          //clk source
        TIMER_A_CLOCKSOURCE_DIVIDER_1,     //divider
        32000,                              //period
        TIMER_A_CAPTURECOMPARE_REGISTER_1,  //compare register
        TIMER_A_OUTPUTMODE_RESET_SET,       //ouput mode
        3200                                // dutyCycle
};

//----------------------------------------------------------------------------------------
// P1.1 (switch) as input with pull-up; enable interrups
void config_IO_Port(){
P1->DIR = ~(uint8_t) BIT1;               // BIT1 = 0
P1->OUT = BIT1;
P1->REN = BIT1;                          // Enable pull-up resistor
P1->SEL0 = 0;
P1->SEL1 = 0;
P1->IES = BIT1;                          // Interrupt on high-to-low transition
P1->IFG = 0;                             // Clear all P1 interrupt flags
P1->IE = BIT1;                           // Enable interrupt for P1.1
NVIC->ISER[1] = 1 << ((PORT1_IRQn) & 31);// Enable Port 1 interrupt on the NVIC
}

//----------------------------------------------------------------------------------------
void ConfigSysTick(void){
    /* Configuring SysTick to trigger at 1500000
    * (MCLK is 3MHz so this will make
    * it toggle every 0.5s) */
    MAP_SysTick_enableModule();
    MAP_SysTick_setPeriod(1500000);
    //MAP_Interrupt_enableSleepOnIsrExit();
    MAP_SysTick_enableInterrupt();
}

bool SysTickTimeUp = false;
void SysTick_Handler(void){
    SysTickTimeUp = true;
    //printf("inISR\n");
}

//----------------------------------------------------------------------------------------

void PORT1_IRQHandler(void){

    volatile uint32_t i;
    //for(i = 0; i < 10000; i++) //debounce
//    printf("In Port ISR\n");
//		printf("AD value = 0x%04X\r\n");
    //register access
    //if interrupt on P1 due to switch connected to P1.1 (switch)
    //toggle LED connected to P1.0
    if(P1->IFG & BIT1) P1->OUT ^= BIT0; //toggle
	
		for(i = 0; i < 10000; i++)
	
    P1->IFG &= ~BIT1;   //clear IFG
    TimerA_Flag = true;
		
    //duplicate of above via driver library; check and clear interrupt on P1
    //uint32_t status = MAP_GPIO_getEnabledInterruptStatus(GPIO_PORT_P1);
    // MAP_GPIO_clearInterruptFlag(GPIO_PORT_P1, status);

    /* Toggle LED */
    //if(status & GPIO_PIN1) MAP_GPIO_toggleOutputOnPin(GPIO_PORT_P1, GPIO_PIN1);
}

//----------------------------------------------------------------------------------------

void setPWM(void){
    if(pwmConfig.dutyCycle == 28800) pwmConfig.dutyCycle = 3200;
    else pwmConfig.dutyCycle += 3200;

    //printf("ISR %d\n", pwmConfig.dutyCycle);
    MAP_Timer_A_generatePWM(TIMER_A0_BASE, &pwmConfig);
}

//---------------------------------------------------------------------------------------
// ignore the rest; just keeping for reference; not used
//----------------------------------------------------------------------------------------

// Initialize SysTick with busy wait running at bus clock.
//void SysTick_Init(void){
//  SYSTICK_STCSR = 0;                    // disable SysTick during setup
//  SYSTICK_STRVR = 0x00FFFFFF;           // maximum reload value
//  SYSTICK_STCVR = 0;                    // any write to current clears it
//  SYSTICK_STCSR = 0x00000005;           // enable SysTick with no interrupts
//}

// (2^24+1)*(1/(3*10^6)) = 5.5924 ; about 6 seconds

// .33333333 micro seconds -> 1 cycle
//    10^6 micro seconds (1 second) -> 333,333.000 cycles
//    333,333 = 0x51615

//void SysTickWait(void){
//    SYSTICK_STRVR = 0x51615;           // maximum reload value; any write to CVR clears it and COUNTFLAG in CSR
//    while((SYSTICK_STCSR&0x00010000) == 0){}
//}



#endif
