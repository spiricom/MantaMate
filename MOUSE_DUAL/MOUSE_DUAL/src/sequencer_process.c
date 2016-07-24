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

uint8_t previous_hex = 0;
uint16_t sequencer_steps[32][10]; // cv1, cv2, keyboard pitch, note/rest, toggled, length
uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = 0;
uint8_t order_vs_pattern = PATTERN;
uint8_t cvouts_vs_steplength = CVOUTS;
uint8_t pattern_type = UPPATTERN;

uint8_t most_recent_hex = 0;
uint8_t current_step = 0;
uint8_t prev_step = 0;
uint8_t step_states[32];
uint8_t num_steps = 16;
uint8_t step_offset = 0;
uint8_t prev_recent_hex = 0;
uint8_t keyboard_pattern[16] = {0,2,4,5,7,9,11,12,1,3,255,6,8,10,254,253};
uint8_t	most_recent_pitch = 0;
uint8_t	prev_recent_pitch = 0;
uint8_t current_seq_octave = 4;
uint8_t prev_keyboard_hex = 210; // some garbage

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
		sequencer_steps[i][5] = 1;  // step_length = 1
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
	current_step++; // has to be incremented at the beginning - not the end, so that other functions know what the real current step is after this function has exited.
	current_step = current_step % num_steps + step_offset;
	manta_set_LED_hex(prev_step, REDOFF);  
	manta_set_LED_hex(prev_step, step_states[prev_step]);  
	manta_set_LED_hex(current_step, RED);  
	
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
	
	// a little fun LED pattern to test things out
	/*
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
	*/
	
	
	//tuningTest(dummycounter % 6);
	//switch (dummycounter % 2)
	//{
	//	case 0: DAC16Send(0, 0); break;
		//case 1: DAC16Send(0, 6553); break;
	//	case 2: DAC16Send(0, 13107); break;
		//case 1: DAC16Send(0, 19661); break;
		//case 1: DAC16Send(0, 26214); break;
	//	case 1: DAC16Send(0, 32768); break;
	//	default: break;
	//}
	prev_step = current_step;
	
	//dummycounter++;

}

void setLEDsForKeyboard(void)
{
	;
}

void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it. 
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
	//check the steps
	for (int i = 0; i < 32; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			most_recent_hex = i;
		}
		pastbutt_states[i] = butt_states[i];
	}
	//check the keyboard selector notes
	for (int i = 32; i < 48; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			most_recent_pitch = keyboard_pattern[i-32];
			if (keyboard_pattern[prev_keyboard_hex-32] < 200)
			{
				manta_set_LED_hex(prev_keyboard_hex, REDOFF);
				manta_set_LED_hex(prev_keyboard_hex, AMBER);
				manta_send_LED();
			}
			else if (keyboard_pattern[prev_keyboard_hex-32] == 255)
			{
				for (int j = 0; j < 16; j++)
				{
					if (keyboard_pattern[j] < 200)
					{
						manta_set_LED_hex(j+32, REDOFF);
						manta_set_LED_hex(j+32, AMBER);
						manta_send_LED();
					}
				}
			}
			
			if (keyboard_pattern[i-32] < 200)
			{
				manta_set_LED_hex(i, AMBEROFF);
				manta_set_LED_hex(i, RED);
				manta_send_LED();
			}
			
			else if (keyboard_pattern[i-32] == 255)
			{
				for (int j = 0; j < 16; j++)
				{
					if (keyboard_pattern[j] < 200)
					{
						manta_set_LED_hex(j+32, AMBEROFF);
						manta_set_LED_hex(j+32, RED);
						manta_send_LED();
					}
				}
			}
			prev_keyboard_hex = i;
		}
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			most_recent_pitch = 252; // code for noteOff
		}
		pastbutt_states[i] = butt_states[i];
	}	
	if (prev_recent_pitch != most_recent_pitch)
	{
		prev_recent_pitch = most_recent_pitch;
		if (most_recent_pitch == 252)
		{
			//ignore note offs
		}
		else if (most_recent_pitch == 255)
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
		}
		
		if (current_step == most_recent_hex)
		{
			DAC16Send(0, ((uint32_t)sequencer_steps[current_step][2]) * 546125 / 1000);
		}
	}
	if (prev_recent_hex != most_recent_hex)
	{
		step_states[prev_recent_hex] = AMBEROFF;
		manta_set_LED_hex(prev_recent_hex, AMBEROFF);
		prev_recent_hex = most_recent_hex;
		manta_set_LED_slider(0,(sequencer_steps[most_recent_hex][0]/512) + 1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(1,(sequencer_steps[most_recent_hex][1]/512) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
	}
	if (current_step != most_recent_hex)
	{
		step_states[most_recent_hex] = AMBER;
		manta_set_LED_hex(most_recent_hex, AMBER);
		manta_send_LED();
	}
	// update the past keymap array (stores the previous values of every key's sensor reading)

}

void processSliderSequencer(uint8_t sliderNum, uint16_t val)
{
	sequencer_steps[most_recent_hex][sliderNum] = val;
	manta_set_LED_slider(sliderNum,(sequencer_steps[most_recent_hex][sliderNum]/512) + 1); // add one to the slider values because a zero turns them off
	manta_send_LED();
	if (current_step == most_recent_hex)
	{
		dacsend(sliderNum, 0, sequencer_steps[current_step][sliderNum]);
	}
}