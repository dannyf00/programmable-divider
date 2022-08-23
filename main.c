//1pps generator
// - generate 1pps from a known external frequency source
// - frequency counting on TMR1/CLKIN pin
// - 12F675 running on EC
//
//
//          +===================+
//          |                   |
//          |               GP0 |
//          |                   |
//          |                   |
//          |         GP5/CLKIN |<------------< Freq input
//          |                   |
//          |                   |
//          |               GP2 |>------------> 1pps output
//          |                   |
//          |                   |
//          |                   |
//          |                   |
//          |                   |
//          +===================+
//

#include "config.h"							//configuration words - INTRCIO mode
#include "gpio.h"                           //we use gpio functions
#include "delay.h"                          //we use software delays
#include "tmr0.h"							//we use tmr0
#include "tmr1.h"							//we use tmr1

//hardware configuration
//1pps output
#define PPS_PORT		GPIO
#define PPS_DDR			TRISIO
#define PPS_PIN			(1<<2)				//1pps signal output

#define FREQ			500000ul			//known frequency of the input signal / clock source
#define FREQ_ERR		18					//need to experiment for your compiler / chip: tmr1 tick elapsed from one run to another
#define FREQ_MSW		((FREQ - FREQ_ERR) >> 16)		//most significant word of FREQ ticks
#define FREQ_LSW		((FREQ - FREQ_ERR) & 0xffff)	//least significant word of FREQ ticks
//frequency / clock input always on GP5/CLKIN
//end hardware configuration

//global defines

//global variables
volatile uint16_t freq_ovf=FREQ_MSW;		//overflow

//isr
//tmr1 is the only isr - very important!
#if 0										//you can run it via interrupt as well
void interrupt isr(void) {
	TMR1IF = 0;								//clear the flag
	if (freq_ovf--==0) {
		//desired time has elapsed - start the next cycle
		TMR1 = FREQ_LSW;					//reload the offset
		freq_ovf = FREQ_MSW;				//initialize freq_ovf
		IO_SET(PPS_PORT, PPS_PIN);			//set the 1pps pin
	} else {
		IO_CLR(PPS_PORT, PPS_PIN);
	}	
}
#endif
	
//initialize frequency calibrator
void pps_init(void) {
	freq_ovf=FREQ_MSW;						//overflow
	
	//pps as output
	IO_CLR(PPS_PORT, PPS_PIN); IO_OUT(PPS_DDR, PPS_PIN);
	
	//start tmr0
	
	//start tmr1
	tmr1_init(TMR1_PS1x, 0xffff);			//count on CLKIN (T1CS=1), 1:1 prescaler
	TMR1ON = 0;								//0->tmr1 off, 1->tmr1 on
	TMR1CS = 1;								//count on pulse train on CLKIN
	nT1SYNC= 1;								//0->tmr1 is synchronized, 1->tmr1 asynchronuous
	TMR1 = -FREQ_LSW;						//start the timer with an offset
	TMR1IF = 0;								//0->clear the flag.
	TMR1IE = 0;								//1->enable the interrupt, 0->disable the interrupt
	//PEIF = 0;								//0->clear the flag
	PEIE = 0;								//1->enable the interrupt, 0->disable the interrupt
	IO_SET(PPS_PORT, PPS_PIN);				//set the output pin
	TMR1ON = 1;								//start TMR1
}
	
int main(void) {
	
	mcu_init();							    //initialize the mcu
	pps_init();								//initialize frequency calibrator
	//ei();									//enable interrupt
	while (1) {
		//this is the only process that the mcu runs on
		if (TMR1IF) {						//tmr1 overflows
			if (freq_ovf--==0) {
				//desired ticks have elapsed - start the next cycle
				TMR1 =-FREQ_LSW;			//reload the offset
				freq_ovf = FREQ_MSW;		//initialize freq_ovf
				IO_SET(PPS_PORT, PPS_PIN);	//set the 1pps pin
			} else {
				//desired ticks have not all elapsed
				IO_CLR(PPS_PORT, PPS_PIN);	//clear the pps pin
			}	
			TMR1IF = 0;						//clear the flag
		}
	}	
}
