#include <avr/io.h>
#include <avr/interrupt.h>

#define T1 0x64;	//tram time (not duration)
#define T2 0x1E;	//tram, pedestrians duration
#define T3 0x0A;	//button's innaction time

int pedestrian_value_enabled = 1;
int T2_duration_passed = 1;


int main(void){

	/* PIN INITIALIZATION */

	//PD0 for pedestrians, PD1 for tram && PD2 for the road
	//set PD0,PD1 to 1 because we are starting with road traffic
	PORTD.DIR |= 0b00000111;
	PORTD.OUT |= 0b00000011;
	
	//PIN5 of PORTF used for the interrupts send from the pedestrians button
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;


	/* TCA0 INITIALIZATION */

	//stop timer and hard reset
	TCA0.SINGLE.CTRLA &= ~(TCA_SINGLE_ENABLE_bm);
	TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc;

	//enable split mode
	TCA0.SPLIT.CTRLD=TCA_SPLIT_SPLITM_bm;

	//Enable timer and prescaler=256, enable LCMP0 for output compare
	//period=T1
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV256_gc | TCA_SPLIT_ENABLE_bm;
	TCA0.SPLIT.CTRLB = TCA_SPLIT_LCMP0EN_bm;
	TCA0.SPLIT.HPER = T1;

	//rising edge on TRAM and enable interrupt
	TCA0.SPLIT.INTCTRL = TCA_SPLIT_HUNF_bm;

	//Initialize values
	TCA0.SPLIT.LCMP0 = 0x00;
	TCA0.SPLIT.HCNT = T1;


	/* INTERRUPTS AND WHILE */

	sei();
	while(1){

	}

}

//ISR FOR T1 (tram passings) 
ISR(TCA0_HUNF_vect){

	//disable interrupts
	cli();

	//clear interrupt flag
	uint8_t interruptFlags = TCA0.SPLIT.INTFLAGS;
	TCA0.SPLIT.INTFLAGS = interruptFlags;

	pedestrian_value_enabled = 0;

	//car lights off PD2
	PORTD.OUT |= 0b00000100;

	//pedestrians (PD0) & tram lights (PD1) turned on
	PORTD.OUTCLR = 0b00000011;

	//enable interrupts from lower bits for T2 interrupt 
	//afterwards that happens on the lower bits
	TCA0.SPLIT.INTCTRL = TCA_SPLIT_LCMP0_bm;
	
	//LCTN to T2 for the next interrupt, the duration interrupt
	TCA0.SPLIT.LCNT = T2;
	
	//enable interrupts again
	sei();

}


//ISR FOR T2 (pedestrians passing time), T3 (duration between button presses)
ISR(TCA0_LCMP0_vect){

	//disable interrupts
	cli();

	//check if tram passes and give it priority
	//HUNF is used in the ISR for tram
	if((TCA0.SPLIT.INTFLAGS & 0b00000010) == 0x02){
		
		//clear flag because hunf is active
		TCA0.SPLIT.INTFLAGS = 0b00000010;
		return;
	}

	//clear interrupt flag
	uint8_t interruptFlags = TCA0.SPLIT.INTFLAGS;
	TCA0.SPLIT.INTFLAGS = interruptFlags;


	//check if T2 has ended and T3 preparation is required
	if(T2_duration_passed){

		//pedestrians & tram leds off
		PORTD.OUT |= 0b00000011;

		//car led turned on
		PORTD.OUTCLR = 0b00000100;
		
		//LCNT to T3 for the next interrupt (duration between button presses)
		TCA0.SPLIT.LCNT = T3;

		T2_duration_passed=0;

	}else{

		pedestrian_value_enabled=1;
		T2_duration_passed=1;

		//enable interrupts from higher bits (T2,T3) because of the split we have
		//the T2,T3 uses the lower 8 bits
		TCA0.SPLIT.INTCTRL = TCA_SPLIT_HUNF_bm;
	}

	//enable interrupts again
	sei();
}


//ISR FOR BUTTON PRESSED
ISR(PORTF_PORT_vect){

	//disable interrupts
	cli();

	//clear interrupt flag
	uint8_t interruptFlags = PORTF.INTFLAGS;
	PORTF.INTFLAGS = interruptFlags;

	if(pedestrian_value_enabled){

		pedestrian_value_enabled = 0;

		//car lights off
		PORTD.OUT |= 0b00000100; 

		//pedestrians lights on
		PORTD.OUTCLR = 0b00000001; 

		//randomize T2,T3 ISR selection because they both 
		//use the last 8 bits
		TCA0.SPLIT.INTCTRL |= TCA_SPLIT_LCMP0_bm; 

		//LCNT to T2 for the next interrupt
		TCA0.SPLIT.LCNT = T2;

	}

	//enable interrupts again
	sei();

}





