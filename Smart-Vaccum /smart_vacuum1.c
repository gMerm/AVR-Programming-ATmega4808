#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SENSOR_THRESHOLD 0x0A			//threshold for checking

uint8_t turn_counter=0;
volatile uint8_t adc_mode = 0;

//for question 2
//turn left, turn right, switch to stop and turn around 
//only 1 adc exists: 1) single conversion for side sensor for T1=1ms
//					 2) free running conversion for front sensor for T2=2ms
//these 1,2 bullets happen in circular way till the full room has been searched
//we should start from single conversion => side sensor checking first
//Left turn = LED2, Right turn = LED0 (without delay)
//When switch 5 is pressed the reverse walking should be enabled


int main(){

	//set LEDS as outputs
	PORTD.DIR |= PIN1_bm;				//LED1 is output (for straight moving)
	PORTD.DIR |= PIN2_bm;				//LED2 is output (for left turn)
	PORTD.DIR |= PIN0_bm;				//LED0 is output (for right turn)

	//initialize ADC for single conversion
	//because we should start from side sensor checking => single conversion
	ADC0.CTRLA |= ADC_RESSEL_10BIT_gc;	//10 bit resolution
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

		//check for adc mode and turn counter
		if(adc_mode==0 && turn_counter < 4){

			ADC0.CTRLE &= ~(ADC_WINCM0_bm);		//clear bit in the control reg
			ADC0.CTRLE |= ADC_WINCM0_bm;		//set bit for comparison
			ADC0.COMMAND |= ADC_STCONV_bm;		//initiates single conversion
			_delay_ms(1);
			adc_mode=1;

		}else if(adc_mode==1 && turn_counter < 4){

			
			ADC0.CTRLE &= ~(ADC_WINCM1_bm);		//clear bit in the control reg
			ADC0.CTRLE |= ADC_WINCM1_bm;		//set bit for comparison
			ADC0.COMMAND |= ADC_FREERUN_bm;		//initiates free running conversion
			_delay_ms(2);
			adc_mode=0;

		}else if(turn_counter >= 4){
			
			cli();
            break;
		}
	}

	return 0;

}

ISR(ADC0_WCOMP_vect) {

	if(adc_mode==0){
		//single conversion mode for side sensor
	    if (ADC0.RES >= SENSOR_THRESHOLD) {
	    	//LED1 off and LED2 on for turning right
	    	PORTD.OUT &= ~PIN1_bm;  		//Turn off LED1
	    	PORTD.OUT |= PIN0_bm;   		//Turn on LED0

	    	//turn counter
	        turn_counter--;

	       	PORTD.OUT |= PIN1_bm;   		//Turn on LED1
	        PORTD.OUT &= ~PIN0_bm;  		//Turn off LED2
	    }
	}else{
		//free running mode for front sensor
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
}
