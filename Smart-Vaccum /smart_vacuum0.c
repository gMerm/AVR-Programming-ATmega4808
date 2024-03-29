#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SENSOR_THRESHOLD 0x0A
uint8_t turn_counter=0;

//ADC takes values from PIN7 of PORTD
//for question 1

int main(){

	//Set PD1 (LED1) as an output for straight movement
	//Set PD2 (LED2) as an output for left turn
	PORTD.DIR |= PIN1_bm;				//LED1 is output
	PORTD.DIR |= PIN2_bm;				//LED2 is output

	//initialize ADC for free-running mode
	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc;	//10 bit resolution
	ADC0.CTRLA |= ADC_FREERUN_bm;		//free running mode enabled
	ADC0.CTRLA |= ADC_ENABLE_bm;		//enable ADC
	ADC0.MUXPOS |= ADC_MUXPOS_AIN7_gc;	//enable the muxpos bit
	ADC0.DBGCTRL |= ADC_DBGRUN_bm;		//enable debug mode


	//Window Comparator Mode (WCM)
	ADC0.WINLT = SENSOR_THRESHOLD; 		//Set threshold
	ADC0.INTCTRL |= ADC_WCMP_bm; 		//Enable Interrupts for WCM
	ADC0.CTRLE |= ADC_WINCM0_bm; 		//Interrupt when RESULT < WINLT

	sei();

	PORTD.OUT |= PIN1_bm;  				//Turn on LED1

	ADC0.COMMAND |= ADC_STCONV_bm; 		//Start Conversion

	while(1){
		//check if the room has been complete
		if(turn_counter >= 4){
			cli();
            break;
        }
	}

	return 0;

}

ISR(ADC0_WCMP_vect) {

    if (ADC0.RES < SENSOR_THRESHOLD) {
        //LED1 off and LED2 on for turning left
        PORTD.OUT &= ~PIN1_bm;  		//Turn off LED1
        PORTD.OUT |= PIN2_bm;   		//Turn on LED2

        //turn counter
        turn_counter++;

        //LED1 on and LED2 off for straight movement
        PORTD.OUT |= PIN1_bm;   		//Turn on LED1
        PORTD.OUT &= ~PIN2_bm;  		//Turn off LED2
    }
}
