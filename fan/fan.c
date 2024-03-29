#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

volatile uint8_t pressed_times = 0;

void TCA0_init(int t1, int t1_dc, int t2, int t2_dc)
{
	//reset, stop timer, hard reset
	TCA0.SINGLE.CTRLA &= ~(TCA_SINGLE_ENABLE_bm);
	TCA0.SINGLE.CTRLESET = TCA_SINGLE_CMD_RESET_gc;

	//enable split mode
	TCA0.SPLIT.CTRLD = TCA_SPLIT_SPLITM_bm;

	// Set prescaler to 256 and enable timer
	TCA0.SPLIT.CTRLA = TCA_SPLIT_CLKSEL_DIV256_gc | TCA_SPLIT_ENABLE_bm;

    //enable both low & high compare channels
    TCA0.SPLIT.CTRLB |= TCA_SPLIT_LCMP0EN_bm | TCA_SPLIT_HCMP0EN_bm;

	//PWM frequencies, duty cycles
	//LPER for blades and t1_value = 1ms and duty cycle 50%
	TCA0.SPLIT.LPER = t1_value; 
	TCA0.SPLIT.LCMP0 = t1_duty_cycle; 
	//HPER for base and t2_value = 2ms and duty cycle 60%
	TCA0.SPLIT.HPER = t2_value; 
	TCA0.SPLIT.HCMP0 = t2_duty_cycle; 

	//check rising edge because this is what the exercise asks for
	TCA0.SPLIT.INTCTRL = TCA_SPLIT_HUNF_bm | TCA_SPLIT_LUNF_bm;
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm | TCA_SPLIT_LUNF_bm;

}


int main(void)
{

    //enable pull-up resistor (PORTF)
	PORTF.PIN5CTRL |= PORT_PULLUPEN_bm | PORT_ISC_BOTHEDGES_gc;

    //set portd output pins for led usage
    PORTD.DIRSET = PIN0_bm | PIN1_bm; 

	sei(); 

	while(1)
	{



	}
}

//isr for blades
//This ISR is triggered when the low part of TCA0 overflows. 
//It toggles pin 0 of port D.
ISR(TCA0_LUNF_vect)
{
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_LUNF_bm;
	//led0
	PORTD.OUTTGL = PIN0_bm;
}

//isr for base
//This ISR is triggered when the high part of TCA0 overflows. 
//It toggles pin 1 of port D.
ISR(TCA0_HUNF_vect)
{
	TCA0.SPLIT.INTFLAGS = TCA_SPLIT_HUNF_bm;
	//led1
	PORTD.OUTTGL = PIN1_bm;
}

//isr for switch 5 that will be used
ISR(PORTF_PORT_vect)
{	
	//dis interrupts and clear flags
    cli();
	PORTF.INTFLAGS = PORT_INT5_bm;

	pressed_times++;
	
	//pressed for the first time
    if(pressed_times == 0){

    	//t1=20, t1 duty cycle (50%) = 20 * 0.5 = 10 (blades)
    	//t2=40, t2 duty cycle (60%) = 40 * 0.6 = 24 (base)
        TCA0_init(20,10,40,24);


    }
    //pressed for the second time
    if(pressed_times == 1){

        //t1=10, t1 duty cycle (50%) = 10 * 0.5 = 5 (blades)
        TCA0_init(10,5,40,24);
        
    }
    //pressed for the third time
    if(pressed_times == 2)
    {

        //disable tca
        TCA0.SPLIT.CTRLA &= ~TCA_SPLIT_ENABLE_bm;
        pressed_times=0;

    }

}








