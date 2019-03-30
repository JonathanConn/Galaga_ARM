#include <ti/devices/msp432p4xx/inc/msp.h>
 #include <ti/devices/msp432p4xx/driverlib/driverlib.h>
 #include <ti/grlib/grlib.h>
 #include "LcdDriver/Crystalfontz128x128_ST7735.h"
 #include <stdio.h>

  /* Graphic library context */
  Graphics_Context g_sContext;

/* ADC results buffer */
static uint16_t oldXY[2];
static uint16_t x, y;

static uint16_t ammo = 10;
static uint16_t score = 0;


static uint16_t OLDammo = 10;
static uint16_t OLDscore = 0;

//radius
static uint16_t r = 5;

//velocity 
static uint16_t v = 1;

//anlge
static uint16_t a = 0;

//traingle 
static uint16_t left[2];
static uint16_t top[2];
static uint16_t right[2];

//traingle for bot 
static uint16_t BOTleft[2];
static uint16_t BOTtop[2];
static uint16_t BOTright[2];

struct bot{
	int x,y,r,h;	
};

struct shot{
	int x,y,r;
};


struct bot bot1;
struct shot shots[1000];
int sCount = 0;

int bCatch = 0;

int main(void) {
  /* Halting WDT and disabling master interrupts */
  MAP_WDT_A_holdTimer();
  MAP_Interrupt_disableMaster();

  /* Set the core voltage level to VCORE1 */
  MAP_PCM_setCoreVoltageLevel(PCM_VCORE1);

  /* Set 2 flash wait states for Flash bank 0 and 1*/
  MAP_FlashCtl_setWaitState(FLASH_BANK0, 2);
  MAP_FlashCtl_setWaitState(FLASH_BANK1, 2);

  /* Initializes Clock System */
  MAP_CS_setDCOCenteredFrequency(CS_DCO_FREQUENCY_48);
  MAP_CS_initClockSignal(CS_MCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
  MAP_CS_initClockSignal(CS_HSMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
  MAP_CS_initClockSignal(CS_SMCLK, CS_DCOCLK_SELECT, CS_CLOCK_DIVIDER_1);
  MAP_CS_initClockSignal(CS_ACLK, CS_REFOCLK_SELECT, CS_CLOCK_DIVIDER_1);

  /* Initializes display */
  Crystalfontz128x128_Init();

  /* Set default screen orientation */
  Crystalfontz128x128_SetOrientation(LCD_ORIENTATION_UP);

  /* Initializes graphics context */
  Graphics_initContext( & g_sContext, & g_sCrystalfontz128x128, & g_sCrystalfontz128x128_funcs);
  Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_RED);
  Graphics_setBackgroundColor( & g_sContext, GRAPHICS_COLOR_BLACK);
  GrContextFontSet( & g_sContext, & g_sFontFixed6x8);
  Graphics_clearDisplay( & g_sContext);

  /* Configures Pin 6.0 and 4.4 as ADC input */
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P6, GPIO_PIN0, GPIO_TERTIARY_MODULE_FUNCTION);
  MAP_GPIO_setAsPeripheralModuleFunctionInputPin(GPIO_PORT_P4, GPIO_PIN4, GPIO_TERTIARY_MODULE_FUNCTION);

  /* Initializing ADC (ADCOSC/64/8) */
  MAP_ADC14_enableModule();
  MAP_ADC14_initModule(ADC_CLOCKSOURCE_ADCOSC, ADC_PREDIVIDER_64, ADC_DIVIDER_8, 0);

  /* Configuring ADC Memory (ADC_MEM0 - ADC_MEM1 (A , A9) with repeat)
   * with internal 2.5v reference */
  MAP_ADC14_configureMultiSequenceMode(ADC_MEM0, ADC_MEM1, true);
  MAP_ADC14_configureConversionMemory(ADC_MEM0,
    ADC_VREFPOS_AVCC_VREFNEG_VSS,
    ADC_INPUT_A15, ADC_NONDIFFERENTIAL_INPUTS);

  MAP_ADC14_configureConversionMemory(ADC_MEM1,
    ADC_VREFPOS_AVCC_VREFNEG_VSS,
    ADC_INPUT_A9, ADC_NONDIFFERENTIAL_INPUTS);

  /* Enabling the interrupt when a conversion on channel 1 (end of sequence)
   * is complete and enabling conversions */
  MAP_ADC14_enableInterrupt(ADC_INT1);

  /* Enabling Interrupts */
  MAP_Interrupt_enableInterrupt(INT_ADC14);
  MAP_Interrupt_enableMaster();

  /* Setting up the sample timer to automatically step through the sequence
   * convert.
   */
  MAP_ADC14_enableSampleTimer(ADC_AUTOMATIC_ITERATION);

  /* Triggering the start of the sample */
  MAP_ADC14_enableConversion();
  MAP_ADC14_toggleConversionTrigger();


	bot1.x = 50;
	bot1.y = 30;
	bot1.r = 5; 
	

  while (1) {
		
		Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_WHITE);
		Graphics_drawLine(& g_sContext, 0, 107, 127, 107); 
		
		 // MAP_PCM_gotoLPM0();
  }
}



void ADC14_IRQHandler(void) {
  uint64_t status;

  status = MAP_ADC14_getEnabledInterruptStatus();
  MAP_ADC14_clearInterruptFlag(status);
	
  if (status & ADC_INT1) {
		//pasing input data from joystick 
		x = ADC14_getResult(ADC_MEM0) / 135;
		if (x + r >= 127) x = 127 - r;
    if (x <= r) x = r;
		
		//the y for our ship never changes 
		y = 127 - (r+25);
		
		//only redraws if ship moves to prevent flickering  
	  if( (oldXY[0] != x) || (oldXY[1] != y) ){	
			Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_BLACK);
			Graphics_drawLine(& g_sContext, left[0], left[1], top[0], top[1]); //left
			Graphics_drawLine(& g_sContext, top[0], top[1],right[0], right[1]); //right
			Graphics_drawLine(& g_sContext, left[0], left[1], right[0], right[1]); //bottom
	  }

    oldXY[0] = x;
    oldXY[1] = y;

		left[0] = x-r;
		left[1] = y+r;
		
		top[0] = x;
		top[1] = y-r;
		
		right[0] = x+r;
		right[1] = y+r;
		
    Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_BLUE);
		Graphics_drawLine(& g_sContext, left[0], left[1], top[0], top[1]); //left
		Graphics_drawLine(& g_sContext, top[0], top[1],right[0], right[1]); //right
		Graphics_drawLine(& g_sContext, left[0], left[1], right[0], right[1]); //bottom

	 //SHOT--------------------------------
    int buttonPressed = 0;
    if (!(P3IN & GPIO_PIN5)) buttonPressed = 1;
		
		//creates a new shot object at ship pos
		if(buttonPressed == 1){
			if(bCatch == 0){
				shots[sCount].x = top[0]+1;
				shots[sCount].y = top[1]; 
				shots[sCount].r = 1;
				sCount++;
			}
		}
	
		//limits fire rate of the button + for slower - for faster 
		if(bCatch >= 5)
			bCatch = 0;
		else
			bCatch++;
		
		//draws each shot in struct array 
		for(int i = 0; i <= sCount; i++){
			Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_WHITE);
			Graphics_fillCircle(& g_sContext, shots[i].x, shots[i].y, shots[i].r);		
		}
		for(int j=50000; j>0; j--);
		for(int i = 0; i <= sCount; i++){
			Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_BLACK);
			Graphics_fillCircle(& g_sContext, shots[i].x, shots[i].y, shots[i].r);
		}
		
		//changes y pos for each shot 
		for(int i = 0; i <= sCount; i++)shots[i].y -= 2;
			
		//SCORE----------------------------
		
		//if a shot is within the x y bounds it adds to score
		for(int i = 0; i <= sCount; i++){	
			if(shots[i].x >= (bot1.x-bot1.r) && shots[i].x <= (bot1.x+bot1.r))
				if(shots[i].y >= (bot1.y-bot1.r) && shots[i].y <= (bot1.y+bot1.r)){
					if(score >= 99) score = 0;
					score++;
				}
		}
		
		char string[10];
		Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_WHITE);
			
		sprintf(string, "SCORE: %d", score);
		Graphics_drawStringCentered(&g_sContext,(int8_t *)string,AUTO_STRING_LENGTH, 30, 115,OPAQUE_TEXT);	

		
		//BOT---------------------------------------		
		Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_BLACK);
		Graphics_drawLine(& g_sContext, BOTleft[0], BOTleft[1], BOTtop[0], BOTtop[1]); //left
		Graphics_drawLine(& g_sContext, BOTtop[0], BOTtop[1],BOTright[0], BOTright[1]); //right
		Graphics_drawLine(& g_sContext, BOTleft[0], BOTleft[1], BOTright[0], BOTright[1]); //bottom	
	
		BOTleft[0] = bot1.x - bot1.r;
		BOTleft[1] = bot1.y - bot1.r;
		
		BOTtop[0] = bot1.x;
		BOTtop[1] = bot1.y + bot1.r;
		
		BOTright[0] = bot1.x + bot1.r;
		BOTright[1] = bot1.y - bot1.r;
		
		Graphics_setForegroundColor( & g_sContext, GRAPHICS_COLOR_RED);
		Graphics_drawLine(& g_sContext, BOTleft[0], BOTleft[1], BOTtop[0], BOTtop[1]); //left
		Graphics_drawLine(& g_sContext, BOTtop[0], BOTtop[1],BOTright[0], BOTright[1]); //right
		Graphics_drawLine(& g_sContext, BOTleft[0], BOTleft[1], BOTright[0], BOTright[1]); //bottom
			
		
		
			
  }
	
}