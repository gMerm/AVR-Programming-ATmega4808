#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SENSOR_THRESHOLD 0x0A			//threshold for checking

uint8_t turn_counter=0;
volatile uint8_t adc_mode = 0;			//used to swap between the 2 ADC modes
volatile uint8_t reverse_mode = 0;		//check if the switch5 has been pressed
volatile uint8_t forward_path[10];		//store the forward path, will be used in reverse path
										//if == 0 in the forward path left turn was made
										//if == 1 in the forward path right turn was made
volatile uint8_t iteration = 0;			//will be used to save info about each turn in the 
										//forward path array & know how many turns we have done
										//in total


//for question 3
//When switch 5 is pressed: 1) stop and turn 180 degrees
//							2) reverse walking should be enabled
//							3) it should stop where it started
//When switch is pressed all LEDS 0,1,2 should be turned on for a time period
//using a timer/counters
//then it follows the path that has covered till now but in reverse

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


	//Setup Switch 5
	PORTF.DIRCLR |= PIN5_bm;				//Set switch PIN as input
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm; 	//Enable pull-up resistor
	PORTF.INT0MASK |= SWITCH_PIN;			//Enable interrupt on the switch pin
	PORTF.INTCTRL |= PORT_INT0LVL_LO_gc;	//set interrupt level to low

	//Setup Timer/Counter 0, will be used in switch for keeping all the leds on
	TCA0.SINGLE.CTRLA |= TC_CLKSEL_DIV64_gc | TC_WGMODE_CTC_gc;		//set TC0 to CTC mode with prescaler of 64
	TCA0.SINGLE.PER = 15625;										//period = (1000 * 10000000) / (64 * 1000) = 15625
	TCA0.SINGLE.INTCTRL |= TC_OVFINTLVL_LO_gc;						//enable interrupts		
	

	sei();

	PORTD.OUT |= PIN1_bm;  				//Turn on LED1
	ADC0.COMMAND |= ADC_STCONV_bm; 		//Start Conversion

	//initialize iteration 
	iteration=0;

	while(1){

		//check for adc mode and turn counter
		if(adc_mode==0 && turn_counter < 4 && reverse_mode==0){

			ADC0.CTRLE &= ~(ADC_WINCM0_bm);		//clear bit in the control reg
			ADC0.CTRLE |= ADC_WINCM0_bm;		//set bit for comparison
			ADC0.COMMAND |= ADC_STCONV_bm;		//initiates single conversion
			_delay_ms(1);
			adc_mode=1;

		}else if(adc_mode==1 && turn_counter < 4 && reverse_mode==0){

			ADC0.CTRLE &= ~(ADC_WINCM1_bm);		//clear bit in the control reg
			ADC0.CTRLE |= ADC_WINCM1_bm;		//set bit for comparison
			ADC0.COMMAND |= ADC_FREERUN_bm;		//initiates free running conversion
			_delay_ms(2);
			adc_mode=0;

		}else if(turn_counter >= 4 && reverse_mode==0){
			
			cli();
            break;
		}

		//check if the reverse path has been enabled
		if(reverse_mode){

			//move based on the saved moves
			for (int i = iteration - 1; i >= 0; i--) {

				//turn right (because the turn was left)
                if (forward_path[i]==0) {

                	//because we are turning right we need the single conversion
                	//start single conversion (side sensor)
                	ADC0.CTRLE &= ~(ADC_WINCM0_bm);		//clear bit in the control reg
					ADC0.CTRLE |= ADC_WINCM0_bm;		//set bit for comparison
					ADC0.COMMAND |= ADC_STCONV_bm;
					_delay_ms(1);
					adc_mode=1;

                } else {
                	//turn left (because the turn was right)
                	//because we are turning right we need the free running conversion
                	//start free running conversion (front sensor)
					ADC0.CTRLE &= ~(ADC_WINCM1_bm);		//clear bit in the control reg
					ADC0.CTRLE |= ADC_WINCM1_bm;		//set bit for comparison
					ADC0.COMMAND |= ADC_FREERUN_bm;		//initiates free running conversion
					_delay_ms(2);
					adc_mode=0;
                }

                //check if it has reached the starting position
                if(iteration==0){
                    break;
                }
            }
		}


	}

	return 0;

}

//ISR for handling interrupts from ADC (left,right turns)
ISR(ADC0_WCMP_vect) {

	if(adc_mode==0){
		//single conversion mode for side sensor 
	    if (ADC0.RES >= SENSOR_THRESHOLD) {
	    	//LED1 off and LED2 on for turning right
	    	PORTD.OUT &= ~PIN1_bm;  		//Turn off LED1
	    	PORTD.OUT |= PIN0_bm;   		//Turn on LED0

	    	//turn counter
	        turn_counter--;

	        //update forward path array that keeps track of the turns
	        iteration++;
	        forward_path[iteration]=1;		//because right turn was made

	       	PORTD.OUT |= PIN1_bm;   		//Turn on LED1
	        PORTD.OUT &= ~PIN0_bm;  		//Turn off LED2
	    }

	}else if(adc_mode==1){

		//free running mode for front sensor
		if (ADC0.RES < SENSOR_THRESHOLD) {
			//LED1 off and LED2 on for turning left
	        PORTD.OUT &= ~PIN1_bm;  		//Turn off LED1
	        PORTD.OUT |= PIN2_bm;   		//Turn on LED2

	        //turn counter
	        turn_counter++;

	       	//update forward path array that keeps track of the turns
	        iteration++;
	        forward_path[iteration]=0;		//because left turn was made

	        //LED1 on and LED2 off for straight movement
	        PORTD.OUT |= PIN1_bm;   		//Turn on LED1
	        PORTD.OUT &= ~PIN2_bm;  		//Turn off LED2
		}

	}

}

//ISR for handling interrupts from switch 5 (reverse path)
ISR(PORTF_INT0_vect) {

	//turn all leds on
    PORTD.OUT |= PIN0_bm;   				//Turn on LED0
    PORTD.OUT |= PIN1_bm;   				//Turn on LED1
    PORTD.OUT |= PIN2_bm;   				//Turn on LED2

    //start timer to keep leds on for specific time
    //an interrupt will stop this
    TCA0.SINGLE.CTRLA |= TC_CLKSEL_DIV64_gc;

}

//ISR for handling timer interrupts for switch 5 (LEDs usage)
ISR(TCA0_OVF_vect) {
    //stop timer
    TCA0.SINGLE.CTRLA &= ~TC_CLKSEL_DIV64_gc;

    //Turn off LEDs
    PORTD.OUT &= ~PIN0_bm;   				//Turn off LED0
    PORTD.OUT &= ~PIN1_bm;   				//Turn off LED1
    PORTD.OUT &= ~PIN2_bm;   				//Turn off LED2

    //enable reverse path
    reverse_mode = 1;

}
