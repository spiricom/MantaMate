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
#define CVOUTS1 0
#define CVOUTS2 1
#define STEPLENGTH 2
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
uint16_t sequencer_steps[32][10]; // cv1, cv2, keyboard pitch, note/rest, toggled, cv3, cv4, octave, length, keyboard_hexagon
uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = RANGEMODE;
uint8_t order_vs_pattern = ORDER;
uint8_t cvouts_vs_steplength = CVOUTS1;
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
uint8_t seq_numnotes = 0;
uint8_t seq_notestack[32];
uint8_t position_in_notestack = 0;
uint8_t stepGo = 1;

void initSequencer(void)
{
	for (int i = 0; i < 32; i++)
	{
		step_states[i] = 0; // not lit
		sequencer_steps[i][0] = 0;  // cv1 zero
		sequencer_steps[i][1] = 0;  // cv2 zero
		sequencer_steps[i][2] = 0;  // keyboard pitch zero
		sequencer_steps[i][3] = 1;  // note, not rest
		sequencer_steps[i][4] = 0;  // not toggled on
		sequencer_steps[i][5] = 0;  // cv3 zero
		sequencer_steps[i][6] = 0;  // cv4 zero
		sequencer_steps[i][7] = 3;  // octave
		sequencer_steps[i][8] = 1;  // step_length = 1
		sequencer_steps[i][9] = 32;  // hexagon number in keyboard range
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


void move_to_next_step(void)
{
	if (order_vs_pattern == PATTERN)
	{
		if (pattern_type == UPPATTERN)
		{
			current_step++;
			current_step = current_step % num_steps + step_offset;
		}
		else if (pattern_type == DOWNPATTERN)
		{
			current_step--;
			if (current_step <= 0)
			{
				current_step = num_steps;
			}
			current_step += step_offset;
		}
	}
	else // otherwise it's "order"
	{
		
		if (seq_numnotes != 0) // if there is at least one note in the stack
		{
			if (position_in_notestack > 0) // if you're not at the most recent note (first one), then go backward in the array (moving from earliest to latest)
			{
				position_in_notestack--;
			}
			else 
			{
				position_in_notestack = (seq_numnotes - 1); // if you are the most recent note, go back to the earliest note in the array
			}
		
			current_step = seq_notestack[position_in_notestack];
			if (current_step == -1)
			{
				stepGo = 0;
			}
			else
			{
				stepGo = 1;
			}
		}
		else
		{
			stepGo = 0;
		}	
		
	}
	
}

void sequencerStep(void)
{

	LED_Toggle(LED5); // turn on the red mantamate panel light
	
	//check if you are supposed to increment yet (based on the length value of the current step)
	step_counter++;
	if (step_counter == sequencer_steps[current_step][8])
	{
		move_to_next_step();
		
		if (stepGo)
		{
			manta_set_LED_hex(prev_step, REDOFF);
			//manta_set_LED_hex(prev_step, step_states[prev_step]);
			manta_set_LED_hex(current_step, REDON);
			
			dacsend(0, 0, sequencer_steps[current_step][0]);
			dacsend(1, 0, sequencer_steps[current_step][1]);
			dacsend(2, 0, sequencer_steps[current_step][5]);
		
			// if this step is not a rest, change the pitch  (should the cv1 and cv2 output changes also be gated by note/rest settings?)
			if (sequencer_steps[current_step][3])
			{
			
				uint32_t DACtemp = (uint32_t)sequencer_steps[current_step][2];
				DACtemp += (sequencer_steps[current_step][7] * 12);
				DACtemp *= 546125;
				DACtemp /= 1000;
				DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
				DAC16Send(2, ((uint32_t)sequencer_steps[current_step][6] * 16)); // tagged CV4 on the 16bit outputs, since I want to use the 12 bits for the gate
			}
			manta_send_LED(); // now write that data to the manta
			
			// to make rests work, we need to have the sequencer send its own clock signal (processed from the input clock)
			// The right way to do this is to set an output HIGH here, and then have an interrupt set it LOW after a certain point - making a nice trigger. For now, we'll set it low and wait a ms and then set it high. This wastes computation time and is not smart, but should work until an interrupt is written.
			dacsend(3, 0, 0);
			seqwait();
			dacsend(3, 0, sequencer_steps[current_step][3] * 4095);
			
			prev_step = current_step;
		}
	step_counter = 0;
		
	}
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
		manta_set_LED_hex(sequencer_steps[most_recent_hex][9], RED);
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


void addNoteToSequencerStack(uint8_t noteVal)
{
	uint8_t j;

	//first move notes that are already in the stack one position to the right
	for (j = seq_numnotes; j > 0; j--)
	{
		seq_notestack[j] = seq_notestack[(j - 1)];
	}

	//then, insert the new note into the front of the stack
	seq_notestack[0] = noteVal;

	seq_numnotes++;	
}

void removeNoteFromSequencerStack(uint8_t noteVal)
{
	uint8_t j,k;
		
	//it's a note-off, remove it from the stack
	//go through the notes that are currently held down to find the one that released
	for (j = 0; j < seq_numnotes; j++)
	{
		//if it's the note that just got released
		if (seq_notestack[j] == noteVal)
		{
			for (k = 0; k < (seq_numnotes - j); k++)
			{
				seq_notestack[k + j] = seq_notestack[k + j + 1];
				//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
				if (k == ((seq_numnotes - j) - 1))
				seq_notestack[k + j + 1] = -1;
			}
			// in case it got put on the stack multiple times
			j--;
			seq_numnotes--;
		}
	}
}

uint8_t toggleSequencerStackNote(uint8_t noteVal)
{
	uint8_t j,k;
	uint8_t foundOne = 0;
	//it's already in the stack, remove it from the stack
	// look through the stack
	for (j = 0; j < seq_numnotes; j++)
	{
		//if you found it
		if (seq_notestack[j] == noteVal)
		{
			for (k = 0; k < (seq_numnotes - j); k++)
			{
				seq_notestack[k + j] = seq_notestack[k + j + 1];
				//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
				if (k == ((seq_numnotes - j) - 1))
				seq_notestack[k + j + 1] = -1;
			}
			// in case it got put on the stack multiple times
			j--;
			seq_numnotes--;
			foundOne = 1;
		}
	}
	if (!foundOne)
	{
		addNoteToSequencerStack(noteVal);
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
		
		if (edit_vs_play == EDITMODE)
		{
			if (prev_recent_hex != most_recent_hex)
			{
				//turn off the amber light for the previously selected sequencer stage hexagon
				//step_states[prev_recent_hex] = AMBEROFF;
				manta_set_LED_hex(prev_recent_hex, AMBEROFF);
				setLEDsForKeyboard();
				prev_recent_hex = most_recent_hex;
				if (cvouts_vs_steplength == CVOUTS1)
				{
					manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][0]/512) + 1); // add one to the slider values because a zero turns them off
					manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][1]/512) + 1); // add one to the slider values because a zero turns them off
				}
				if (cvouts_vs_steplength == CVOUTS2)
				{
					manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][5]/512) + 1); // add one to the slider values because a zero turns them off
					manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][6]/512) + 1); //add one to the slider values because a zero turns them off/
				}
				else
				{
					manta_set_LED_slider(0,sequencer_steps[most_recent_hex][7] + 1); // OCTAVE add one to the slider values because a zero turns them off
					manta_set_LED_slider(1,sequencer_steps[most_recent_hex][8]); // the step length is already between 1-8
				}
					
				//turn the amber light on for the currently selected sequencer stage hexagon
				//step_states[most_recent_hex] = AMBERON;
				manta_set_LED_hex(most_recent_hex, AMBERON);
				manta_send_LED();
			}
		}
		//otherwise we are in "play" mode and we want our touches to control which steps the sequencer can step on
		else
		{
			if (arp_vs_seq == SEQMODE) // note ons should toggle sequencer steps in and out of the pattern
			{

				{
					toggleSequencerStackNote(most_recent_hex);
				}
			}
			else  // "arp mode", note ons should add to pattern, note offs should remove from pattern, so pattern only sounds when fingers are down (not sure if this is useful)
			{
				// need to create a "note off" message in order to do this properly
			}
							
			//turn the amber light on for the currently selected sequencer stage hexagon
			//step_states[most_recent_hex] = AMBERON;
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
			if (current_seq_octave > 7)
			{
				current_seq_octave = 7;
			}
		}
		else
		{
			sequencer_steps[most_recent_hex][3] = 1;
			sequencer_steps[most_recent_hex][2] = most_recent_pitch;
			//sequencer_steps[most_recent_hex][2] = most_recent_pitch + (current_seq_octave * 12);
			sequencer_steps[most_recent_hex][7] = current_seq_octave;
			sequencer_steps[most_recent_hex][9] = most_recent_upper_hex;
		}
		
		
		if (current_step == most_recent_hex)
		{
			//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
			uint32_t DACtemp = (uint32_t)sequencer_steps[current_step][2];
			DACtemp += (sequencer_steps[current_step][7] * 12);
			DACtemp *= 546125;
			DACtemp /= 1000;
			DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
		}
		
		setLEDsForKeyboard();
		
		//set memory variables
		new_upper_hex = 0;
		prev_keyboard_hex = most_recent_upper_hex;
		prev_recent_pitch = most_recent_pitch;
	}
	
	if (new_func_button)
	{
		
		//make this toggle three different possibilities
		if (most_recent_func_button == 0)
		{
			if (cvouts_vs_steplength == CVOUTS1)
			{
				cvouts_vs_steplength = CVOUTS2;
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][5]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][6]/512) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_button(0, AMBER);
				manta_send_LED();
			}
			else if (cvouts_vs_steplength == CVOUTS2)
			{
				cvouts_vs_steplength = STEPLENGTH;
				manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][7]) + 1); // add one to the slider values because a zero turns them off
				manta_set_LED_slider(1,sequencer_steps[most_recent_hex][8]); // the step length is already between 1-8
				manta_set_LED_button(0, RED);
				manta_send_LED();
			}
			else
			{
				cvouts_vs_steplength = CVOUTS1;
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
	if (cvouts_vs_steplength == CVOUTS1)
	{
		sequencer_steps[most_recent_hex][sliderNum] = val;
		manta_set_LED_slider(sliderNum,(sequencer_steps[most_recent_hex][sliderNum]/512) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (current_step == most_recent_hex)
		{
			dacsend(sliderNum, 0, sequencer_steps[current_step][sliderNum]);
		}
	}
	// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
	else if (cvouts_vs_steplength == CVOUTS2)
	{
		sequencer_steps[most_recent_hex][sliderNum + 5] = val;
		manta_set_LED_slider(sliderNum,(sequencer_steps[most_recent_hex][sliderNum + 5]/512) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (current_step == most_recent_hex)
		{
			dacsend((sliderNum + 2), 0, sequencer_steps[current_step][sliderNum + 5]);
		}
	}
	else // otherwise, you're in third slider mode, where top slider is octave and bottom slider is note length
	{
		if (sliderNum == 0)
		{
			uint16_t prevOct = sequencer_steps[most_recent_hex][7];
			uint16_t newOct = (val / 512);
			sequencer_steps[most_recent_hex][7] = newOct;
			manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][7]) + 1); // add one to the slider values because a zero turns them off
			manta_send_LED();
			if ((current_step == most_recent_hex) && (prevOct != newOct))
			{
				uint32_t DACtemp = (uint32_t)sequencer_steps[current_step][2];
				DACtemp += (sequencer_steps[current_step][7] * 12);
				DACtemp *= 546125;
				DACtemp /= 1000;
				DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
		}
		else
		{
			sequencer_steps[most_recent_hex][8] = (val/512) + 1; //step length (should be 1-8)
			manta_set_LED_slider(1,sequencer_steps[most_recent_hex][8]); // add one to the slider values because a zero turns them off
			manta_send_LED();
		}
		
	}
}