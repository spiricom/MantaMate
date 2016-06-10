/*
 * sequencer_process.c
 *
 * Created: 6/10/2016 1:44:39 PM
 *  Author: Jeff Snyder
 */ 
#include "asf.h"
#include "main.h"

void sequencerStep(void)
{
	// Hey Reid, put some sequencer stuff in here! This should take every metronome click or gate signal in and decide what to do (i.e., skip this step, or compute an output)
	// right now, every sequencer clock it will toggle the red light on the MantaMate on or off.

	if(dummycounter % 2 == 1)
	{
		LED_On(LED5);
	}
	else
	{
		LED_Off(LED5);
	}
	dummycounter++;

}

void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it. 
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
}