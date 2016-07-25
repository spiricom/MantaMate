/*
 * sequencer_process.c
 *
 * Created: 6/10/2016 1:44:39 PM
 *  Author: Jeff Snyder
 */ 
#include "asf.h"
#include "main.h"
#include "note_process.h"
#include "uhi_hid_manta.h"

//names of sequencer step choice modes
#define ORDER 1
#define PATTERN 0
#define CVOUTS 0
#define STEPLENGTH 1
#define UPPATTERN 0
#define DOWNPATTERN 1
#define UPDOWNPATTERN 2
#define RANDOMPATTERN 3
#define RANDOMWALKPATTERN 4
#define EDITMODE 0
#define PLAYMODE 1
#define RANGEMODE 0
#define TOGGLEMODE 1
#define SINGLEMODE 0
#define DUALMODE 1
#define KEYMODE 0
#define MODESELECT 1
#define SEQMODE 0
#define ARPMODE 1

uint8_t previous_hex = 0;
uint16_t sequencer_steps[32][10]; // cv1, cv2, keyboard pitch, note/rest, toggled, cv3, length, keyboard_hexagon
uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = RANGEMODE;
uint8_t order_vs_pattern = PATTERN;
uint8_t cvouts_vs_steplength = CVOUTS;
uint8_t pattern_type = UPPATTERN;
uint8_t edit_vs_play = EDITMODE;
uint8_t single_vs_dual = SINGLEMODE;
uint8_t key_vs_modeselect = KEYMODE;
uint8_t arp_vs_seq = SEQMODE;

uint8_t most_recent_hex = 0;
uint8_t current_step = 0;
uint8_t prev_step = 0;
uint8_t step_states[32];
uint8_t num_steps = 32;
uint8_t step_offset = 0;
uint8_t prev_recent_hex = 0;
uint8_t keyboard_pattern[16] = {0,2,4,5,7,9,11,12,1,3,255,6,8,10,254,253};
uint8_t	most_recent_pitch = 0;
uint8_t	prev_recent_pitch = 0;
uint8_t current_seq_octave = 4;
uint8_t prev_keyboard_hex = 210; // some garbage
uint8_t most_recent_upper_hex = 0;
uint8_t most_recent_func_button = 0;
uint8_t new_upper_hex = 0;
uint8_t new_lower_hex = 0;
uint8_t new_func_button = 0;
uint8_t step_counter = 0;

void initSequencer(void)
{
	for (int i = 0; i < 32; i++)
	{
		step_states[i] = 0; // not lit
		sequencer_steps[i][0] = 0;  // cv1 zero
		sequencer_steps[i][1] = 0;  // cv2 zero
		sequencer_steps[i][2] = 48;  // keyboard pitch zero
		sequencer_steps[i][3] = 1;  // note, not rest
		sequencer_steps[i][4] = 0;  // not toggled on
		sequencer_steps[i][5] = 0;  // cv3 zero
		sequencer_steps[i][6] = 1;  // step_length = 1
		sequencer_steps[i][7] = 32;  // hexagon number in keyboard range
	}
	for (int i = 0; i < 16; i++)
	{
		if (keyboard_pattern[i] < 200)
		{
			manta_set_LED_hex(i+32, AMBER); 
		}
	}
	
	manta_send_LED(); // now write that data to the manta 
}
void seqwait(void)
{
	static uint8_t i = 0;
	static uint8_t wastecounter = 0;
	//cpu_delay_us(12,64000000);//5
	for (i = 0; i < 10; i++) //number arrived at by testing when the timing makes the DAC fail
	{
		wastecounter++;
	}
}

void sequencerStep(void)
{
	// Hey Reid, put some sequencer stuff in here! This should take every metronome click or gate signal in and decide what to do (i.e., skip this step, or compute an output)
	// right now, every sequencer clock it will toggle the red light on the MantaMate on or off, and advance a red led on the manta one step further.


	LED_Toggle(LED5); // turn on the red mantamate panel light
	
	//check if you are supposed to increment yet (based on the length value of the current step)
	step_counter++;
	if (step_counter == sequencer_steps[current_step][6])
	{
		current_step++; // has to be incremented at the beginning - not the end, so that other functions know what the real current step is after this function has exited.
		current_step = current_step % num_steps + step_offset;
		manta_set_LED_hex(prev_step, REDOFF);
		manta_set_LED_hex(prev_step, step_states[prev_step]);
		manta_set_LED_hex(current_step, REDON);
			
		dacsend(0, 0, sequencer_steps[current_step][0]);
		dacsend(1, 0, sequencer_steps[current_step][1]);
		// if this step is not a rest, change the pitch  (should the cv1 and cv2 output changes also be gated by note/rest settings?)
		if (sequencer_steps[current_step][3])
		{
			DAC16Send(0, ((uint32_t)sequencer_steps[current_step][2]) * 54613 / 100);
		}
		manta_send_LED(); // now write that data to the manta
			
		// to make rests work, we need to have the sequencer send its own clock signal (processed from the input clock)
		// The right way to do this is to set an output HIGH here, and then have an interrupt set it LOW after a certain point - making a nice trigger. For now, we'll set it low and wait a ms and then set it high. This wastes computation time and is not smart, but should work until an interrupt is written.
		dacsend(3, 0, 0);
		seqwait();
		dacsend(3, 0, sequencer_steps[current_step][3] * 4095);
			
		prev_step = current_step;
		step_counter = 0;
	}

	
	//dummycounter++;

}

void setLEDsForKeyboard(void)
{
	if (sequencer_steps[most_recent_hex][3] == 1)
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+32, AMBER);
			}
		}
		manta_set_LED_hex(sequencer_steps[most_recent_hex][7], RED);
		manta_send_LED();
	}
	
	
	else
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+32, RED);
				
			}
		}
		manta_send_LED();
	}
}

void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it. 
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
	int i = 0;
	
	//check the sequencer step hexagons
	for (i = 0; i < 32; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			most_recent_hex = i;
			new_lower_hex = 1;
		}
		pastbutt_states[i] = butt_states[i];
	}
	
	//check the upper keyboard selector notes
	for (i = 32; i < 48; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			//an upper hexagon was just pressed
			most_recent_upper_hex = i;
			new_upper_hex = 1;
		}
		pastbutt_states[i] = butt_states[i];
	}	
	
	for (i = 0; i < 4; i++)
	{
		if ((func_button_states[i] > 0) && (past_func_button_states[i] <= 0))
		{
			//a round function button was just pressed
			//this code currently doesn't function correctly if two buttons are pressed within the same 6ms window of the Manta scan. TODO: we should make sure it counts how many are waiting - have a stack instead of one item of storage.
			most_recent_func_button = i;
			new_func_button = 1;
		}
		past_func_button_states[i] = func_button_states[i];
	}
	
	//did you find a new lower hexagon?
	if (new_lower_hex)
	{
		if (prev_recent_hex != most_recent_hex)
		{
			//turn off the amber light for the previously selected sequencer stage hexagon
			step_states[prev_recent_hex] = AMBEROFF;
			manta_set_LED_hex(prev_recent_hex, AMBEROFF);
			setLEDsForKeyboard();
			prev_recent_hex = most_recent_hex;
			if (cvouts_vs_steplength == CVOUTS)
			{
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][0]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][1]/512) + 1); // add one to the slider values because a zero turns them off
			}
			else
			{
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][5]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,sequencer_steps[most_recent_hex][6]); // the step length is already between 1-8
			}
			
			//turn the amber light on for the currently selected sequencer stage hexagon
			step_states[most_recent_hex] = AMBERON;
			manta_set_LED_hex(most_recent_hex, AMBERON);
			manta_send_LED();
		}
		
		new_lower_hex = 0;
	}
	
	// did you find a new upper hexagon?
	if (new_upper_hex)
	{
		most_recent_pitch = keyboard_pattern[most_recent_upper_hex-32];
		
		if (most_recent_pitch == 255)
		{
			//make a rest
			sequencer_steps[most_recent_hex][3] = 0;
		}
		else if (most_recent_pitch == 254)
		{
			// down an octave
			current_seq_octave -= 1;
			if (current_seq_octave < 0)
			{
				current_seq_octave = 0;
			}
		}
		else if (most_recent_pitch == 253)
		{
			//up an octave
			current_seq_octave += 1;
			if (current_seq_octave > 10)
			{
				current_seq_octave = 10;
			}
		}
		else
		{
			sequencer_steps[most_recent_hex][3] = 1;
			sequencer_steps[most_recent_hex][2] = most_recent_pitch + (current_seq_octave * 12);
			sequencer_steps[most_recent_hex][7] = most_recent_upper_hex;
		}
		
		
		if (current_step == most_recent_hex)
		{
			//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
			DAC16Send(0, ((uint32_t)sequencer_steps[current_step][2]) * 546125 / 1000);
		}
		
		setLEDsForKeyboard();
		
		//set memory variables
		new_upper_hex = 0;
		prev_keyboard_hex = most_recent_upper_hex;
		prev_recent_pitch = most_recent_pitch;
	}
	
	if (new_func_button)
	{
		if (most_recent_func_button == 0)
		{
			if (cvouts_vs_steplength == CVOUTS)
			{
				cvouts_vs_steplength = STEPLENGTH;
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][5]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,sequencer_steps[most_recent_hex][6]); // the step length is already between 1-8
				manta_set_LED_button(0, AMBER);
				manta_send_LED();
			}
			else
			{
				cvouts_vs_steplength = CVOUTS;
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][0]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][1]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_button(0, OFF);
				manta_send_LED();
			}
			
		}
		else if (most_recent_func_button == 1)
		{
			if (edit_vs_play == EDITMODE)
			{
				edit_vs_play = PLAYMODE;
				manta_set_LED_button(1, AMBER);
				manta_send_LED();
			}
			else
			{
				edit_vs_play = EDITMODE;
				manta_set_LED_button(1, OFF);
				manta_send_LED();
			}	
			
		}
		else if (most_recent_func_button == 2)
		{
			if (key_vs_modeselect == KEYMODE)
			{
				key_vs_modeselect = MODESELECT;
				manta_set_LED_button(2, RED);
				manta_send_LED();
			}
			else
			{
				key_vs_modeselect = KEYMODE;
				manta_set_LED_button(2, OFF);
				manta_send_LED();
			}			
		}
		else if (most_recent_func_button == 3)
		{
			if (arp_vs_seq == ARPMODE)
			{
				arp_vs_seq = SEQMODE;
				manta_set_LED_button(3, RED);
				manta_send_LED();
			}
			else
			{
				arp_vs_seq = ARPMODE;
				manta_set_LED_button(3, OFF);
				manta_send_LED();
			}			
		}
		
		new_func_button = 0;
	}

}

void processSliderSequencer(uint8_t sliderNum, uint16_t val)
{
	if (cvouts_vs_steplength == CVOUTS)
	{
		sequencer_steps[most_recent_hex][sliderNum] = val;
		manta_set_LED_slider(sliderNum,(sequencer_steps[most_recent_hex][sliderNum]/512) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (current_step == most_recent_hex)
		{
			dacsend(sliderNum, 0, sequencer_steps[current_step][sliderNum]);
		}
	}
	else // otherwise, you're in alternate slider mode, where top slider is cv3 out and bottom slider is note length
	{
		if (sliderNum == 0)
		{
			sequencer_steps[most_recent_hex][5] = val; //cv3 out
			manta_set_LED_slider(sliderNum,(sequencer_steps[most_recent_hex][sliderNum+5]/512) + 1); // add one to the slider values because a zero turns them off
			manta_send_LED();
			if (current_step == most_recent_hex)
			{
				dacsend(2, 0, sequencer_steps[current_step][sliderNum+5]);
			}
		}
		else
		{
			sequencer_steps[most_recent_hex][6] = (val/512) + 1; //step length (should be 1-8)
			manta_set_LED_slider(sliderNum,sequencer_steps[most_recent_hex][6]); // add one to the slider values because a zero turns them off
		}
		
	}
}