/*
 * sequencer_process.c
 *
 * Created: 6/10/2016 1:44:39 PM
 *  Author: Jeff Snyder
 */ 
#include "asf.h"
#include "main.h"
#include "note_process.h"

uint8_t previous_hex = 0;

void sequencerStep(void)
{
	// Hey Reid, put some sequencer stuff in here! This should take every metronome click or gate signal in and decide what to do (i.e., skip this step, or compute an output)
	// right now, every sequencer clock it will toggle the red light on the MantaMate on or off, and advance a red led on the manta one step further.


	LED_Toggle(LED5); // turn on the red mantamate panel light
	manta_set_LED_hex((dummycounter % 48), RED);  
	manta_set_LED_hex(((dummycounter + 1) % 48), AMBER);  
	//turn off the previous LED
	manta_set_LED_hex(previous_hex, OFF);  

	manta_set_LED_slider(0,(dummycounter % 8) + 1); // add one to the slider values because a zero turns them off
	manta_set_LED_slider(1,(7 - (dummycounter % 8)) + 1); // add one to the slider values because a zero turns them off - this one goes backwards
	
	manta_set_LED_button(((dummycounter + 1) % 4),AMBER);
	manta_set_LED_button((dummycounter % 4),OFF);
	manta_send_LED(); // now write that data to the manta
	previous_hex = dummycounter % 48;
	
	//tuningTest(dummycounter % 6);
	dummycounter++;

}

void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it. 
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
}