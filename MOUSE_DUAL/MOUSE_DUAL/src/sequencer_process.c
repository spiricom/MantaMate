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

#include "sequencer.h"

// Typedef versions of Manta modes.

//------------------  S T R U C T U R E S  -------------------
typedef enum MantaSequencer {
	SequencerOne = 0,
	SequencerTwo,
	SequencerNil,
}MantaSequencer;
	
typedef enum MantaEditPlayMode {
	EditMode = 0,
	PlayToggleMode,
	MantaEditPlayModeNil
}MantaEditPlayMode;

typedef enum MantaSeqArpMode {
	SeqMode = 0,
	ArpMode,
	MantaSeqArpModeNil
}MantaSeqArpMode;

typedef enum MantaKeySelectMode {
	KeyMode = 0,
	SelectMode,
	MantaKeySelectModeNil
}MantaKeySelectMode;


typedef enum MantaSingleDualMode {
	SingleMode = 0,
	DualMode,
	MantaSingleDualModeNil
}MantaSingleDualMode;

typedef enum MantaRangeToggleMode {
	RangeMode = 0,
	ToggleMode,
	MantaRangeToggleModeNil
}MantaRangeToggleMode;

/* Replaced these defines with enum:
#define CVOUTS1 0
#define CVOUTS2 1
#define STEPLENGTH 2
*/
typedef enum MantaSliderMode {
	SliderModeOne = 0, //CV1, CV2
	SliderModeTwo,     //CV3, CV4
	SliderModeThree,   //OCTAVE, STEPLENGTH
	SliderModeNil
}MantaSliderMode;

typedef enum MantaSlider {
	SliderOne = 0,
	SliderTwo,
	SliderNil
}MantaSlider;

typedef enum MantaButtonXY {
	ButtonTopLeft = 0,
	ButtonTopRight = 1,
	ButtonBottomLeft = 2,
	ButtonBottomRight = 3,
	SliderModeButton = ButtonTopLeft,
	OptionModeButton = ButtonBottomLeft,
	EditToggleButton = ButtonTopRight,
	PlayModeButton2 = ButtonBottomRight,
	ButtonNil
}MantaButton;

//names of sequencer step choice modes
#define ORDER 1
#define PATTERN 0

MantaSequencer currentSequencer = SequencerOne;

uint8_t prev_pattern_hex = 0;
uint8_t most_recent_pattern_hex = 0;

uint8_t prev_panel_hex = 0;
uint8_t most_recent_panel_hex = 0;

uint8_t prev_option_hex = 0;
uint8_t most_recent_option_hex = 0;

// Sequencer patterns to be used in the program
#define NUM_PATTERNS 8


typedef enum GlobalOptionType
{
	FullMode,
	SplitMode,
	PitchMode,
	TriggerMode,
} GlobalOptionType;

#define NUM_GLOBAL_OPTIONS 4

GlobalOptionType globalOptionTypes[NUM_GLOBAL_OPTIONS] = {
	FullMode,
	SplitMode,
	PitchMode,
	TriggerMode,
};

typedef enum PanelSwitch
{
	PanelRight,
	PanelLeft,
	PanelNil,
} PanelSwitch;

#define NUM_PANEL_MOVES 2

PanelSwitch panelSwitch[NUM_PANEL_MOVES] = {
	PanelLeft,
	PanelRight,
};

SequencerPatternType pattern_type = LeftRightRowUp;

#define MAX_STEPS 32

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
#define OPTIONMODE 1
#define SEQMODE 0
#define ARPMODE 1

// INPUT
void processTouchFunctionButton(MantaButton button);
void processTouchLowerHex(uint8_t hexagon);
void processTouchUpperHex(uint8_t hexagon);

// LEDs
void setSliderLEDsFor(uint8_t hexagon);
void setKeyboardLEDsFor(uint8_t hexagon);
void setModeLEDsFor(MantaSequencer seq);
void setSequencerLEDsFor(MantaSequencer seq);

// NOTE STACK
uint8_t toggleSequencerStackNote(uint8_t noteVal);
void removeNoteFromSequencerStack(uint8_t noteVal);
void addNoteToSequencerStack(uint8_t noteVal);
int moveToNextStackNote(void);

// UTILITIES
void seqwait(void);

/* - - - - - - KEY PATTERNS - - - - - - - -*/
// Upper keyboard pattern
uint8_t keyboard_pattern[16] = {0,2,4,5,7,9,11,12,1,3,255,6,8,10,254,253};
	
// Additional options pattern
uint8_t option_pattern[16] = {2,2,2,2,2,2,2,2,1,1,1,1,0,0,3,3};
	
// UpDown pattern


/* - - - - - - - - - - - - - - - - - - - - */

uint8_t previous_hex = 0;

#define NUM_SEQ 2
tSequencer32 sequencer[NUM_SEQ]; 

// The steps
tStep sequencer_steps[MAX_STEPS];
tNoteStack32 notestack; 

#if STACK_OLD
uint8_t seq_numnotes = 0;
uint8_t seq_notestack[MAX_STEPS];
uint8_t position_in_notestack = 0;
#endif

MantaSliderMode mantaSliderMode = SliderModeOne;
MantaButton most_recent_func_button = ButtonTopLeft;

uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = TOGGLEMODE;
uint8_t order_vs_pattern = PATTERN;
uint8_t edit_vs_play = EDITMODE;
uint8_t single_vs_dual = SINGLEMODE;
uint8_t key_vs_option = KEYMODE;
uint8_t arp_vs_seq = SEQMODE;

uint8_t most_recent_hex = 0;
int8_t current_step = 0;
uint8_t prev_step = 0;
uint8_t step_states[MAX_STEPS];
uint8_t num_steps = MAX_STEPS;
uint8_t step_offset = 0;
uint8_t prev_recent_hex = 0;

	
uint8_t	most_recent_pitch = 0;
uint8_t	prev_recent_pitch = 0;
int8_t current_seq_octave = 4;
uint8_t prev_keyboard_hex = 210; // some garbage
uint8_t most_recent_upper_hex = 0;

uint8_t new_upper_hex = 0;
uint8_t new_lower_hex = 0;
uint8_t new_func_button = 0;
uint8_t step_counter = 0;

uint8_t stepGo = 1;

extern uint8_t func_button_states[4];

#define sequencerGet(SEQ,STEP,PARAM) SEQ.get(&SEQ,STEP,PARAM)
#define sequencerSet(SEQ,STEP,PARAM,VAL) SEQ.get(&SEQ,STEP,PARAM,VAL)

#define toggleNoteInStack(SEQ,NOTE)	sequencer[SEQ].notestack.toggle(&sequencer[SEQ].notestack,NOTE)

#define STACK_OLD 0
#define SEQ_OLD 0
int octave_test = 0;
void initSequencer(void)
{
	//tNoteStack32Init(&notestack);
	
	currentSequencer = 0;
	for (int i = 0; i < NUM_SEQ; i++)
	{
		tSequencer32Init(&sequencer[i]);
	}
	

	
	setKeyboardLEDsFor(0);
	setSequencerLEDsFor(0);
	
	manta_send_LED(); // now write that data to the manta 
}
int lencounter = 0;
int lenToMatch = 0;
int cstep, curr, numnotes;
void sequencerStep(void)
{

	LED_Toggle(LED1); // turn on the red mantamate panel light (should this be LED5?)
	
	//check if you are supposed to increment yet (based on the length value of the current step)

	int offset = 0;
	
	for (int i = 0; i < NUM_SEQ; i++)
	{
		offset = i * 2;
		
		cstep = sequencer[i].currentStep;
		
		sequencer[i].lengthCounter += 1;
		
		if (sequencer[i].lengthCounter == sequencer[i].step[cstep].length)
		{
			sequencer[i].next(&sequencer[i]); // Move to next step, seq 1.
			
			curr = sequencer[i].currentStep;
			
			if (i == currentSequencer)
			{
				prev_step = sequencer[i].prevStep;
				current_step = curr;
			}
			
			if (sequencer[i].stepGo)
			{
				dacsend(offset+0, 0, sequencer[i].step[curr].cv1);
				dacsend(offset+1, 0, sequencer[i].step[curr].cv2);
				dacsend(offset+0, 1, sequencer[i].step[curr].cv3);
				dacsend(offset+1, 1, sequencer[i].step[curr].cv4);

				// if this step is not a rest, change the pitch  (should the cv1 and cv2 output changes also be gated by note/rest settings?)
				if (sequencer[i].step[curr].note == 1)
				{
					uint32_t DACtemp = ((uint32_t)sequencer[i].step[curr].pitch);
					DACtemp += (sequencer[i].step[curr].octave * 12);
					DACtemp *= 546125;
					DACtemp /= 1000;
					DAC16Send(offset+0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
					//DAC16Send(offset+2, ((uint32_t)sequencer[i].step[curr].cv4 * 16)); // tagged CV4 on the 16bit outputs, since I want to use the 12 bits for the gate
				}
				
				// to make rests work, we need to have the sequencer send its own clock signal (processed from the input clock)
				// The right way to do this is to set an output HIGH here, and then have an interrupt set it LOW after a certain point - making a nice trigger.
				// For now, we'll set it low and wait a ms and then set it high. This wastes computation time and is not smart, but should work until an interrupt is written.
				
				//DAC16Send(offset+1, 0);
				//seqwait();
				//DAC16Send(offset+1, sequencer[i].step[curr].note * 65535);
			}
			sequencer[i].lengthCounter = 0;
		}	
	}
	
	// UI
	if (edit_vs_play == EDITMODE)
	{
		
		if (current_step == most_recent_hex)
		{
			manta_set_LED_hex(current_step, RedOff);
			manta_set_LED_hex(current_step, AmberOn);
		}
		else
		{
			manta_set_LED_hex(current_step, RedOn);
		}
		
		if (prev_step == most_recent_hex)
		{
			manta_set_LED_hex(prev_step, AmberOff);
			manta_set_LED_hex(prev_step, RedOn);
		}
		else
		{
			manta_set_LED_hex(prev_step, RedOff);
		}
	}
	else
	{
		manta_set_LED_hex(prev_step, RedOff);
		manta_set_LED_hex(current_step, RedOn);
	}
	manta_send_LED(); // now write that data to the manta
}

void setSequencerLEDsFor(MantaSequencer seq)
{
	// Rough implementation
	for (int i = 0; i < 32; i++)
	{
		manta_set_LED_hex(i,Off);
		if (sequencer[seq].step[i].toggled == 1)
		{
			manta_set_LED_hex(i,AmberOn);
			
			if (i == sequencer[seq].currentStep)
			{
				manta_set_LED_hex(i,RedOn);
				if (edit_vs_play == EDITMODE &&  i == most_recent_hex)
				{
					manta_set_LED_hex(i,AmberOff);	
				}

			}
		}
		else
		{
			manta_set_LED_hex(i,AmberOff);
		}
	}
	return;
}

void setKeyboardLEDsFor(uint8_t hexagon)
{
	if (sequencer[currentSequencer].step[hexagon].note == 1)
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+32, Amber);
			}
			else
			{
				manta_set_LED_hex(j+32, Off);
			}
		}
		manta_set_LED_hex(sequencer[currentSequencer].step[hexagon].hexagon, Red);
	}
	else
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+32, Red);
			} 
			else if (keyboard_pattern[j] == 255)
			{
				manta_set_LED_hex(j+32,Amber);
			}
			else
			{
				manta_set_LED_hex(j+32, Off);
			}
		}
	}
	
	manta_set_LED_hex(62,Off);
	manta_set_LED_hex(63,Off);
}

void setModeLEDsFor(MantaSequencer seq)
{
	//change the keyboard LEDs to be the MODE leds
	for (int i = 0; i < 16; i++)
	{
		if (option_pattern[i] == 0)
		{
			manta_set_LED_hex(i+32, Off);
		}
		else if (option_pattern[i] == 1)
		{
			manta_set_LED_hex(i+32, Amber);
			
		}
		else if (option_pattern[i] == 2 )
		{
			if (i == sequencer[seq].pattern)
			{
				manta_set_LED_hex(i+32, Red);
			}
			else
			{
				manta_set_LED_hex(i+32, Amber);
			}
		}
		else if (option_pattern[i] == 3)
		{
			if ((i-14) == seq)
			{
				manta_set_LED_hex(i+32,Amber);
			}
			else
			{
				manta_set_LED_hex(i+32,Red);
			}
			
		}
		
	}
	
	
}

void setSliderLEDsFor(uint8_t hexagon)
{
	// CV OUTS
	if (mantaSliderMode == SliderModeOne)
	{
		manta_set_LED_slider(SliderOne, (sequencer[currentSequencer].step[hexagon].cv1 >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,(sequencer[currentSequencer].step[hexagon].cv2 >> 9) + 1); // add one to the slider values because a zero turns them off
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		manta_set_LED_slider(SliderOne,(sequencer[currentSequencer].step[hexagon].cv3 >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,(sequencer[currentSequencer].step[hexagon].cv4 >> 9) + 1); //add one to the slider values because a zero turns them off/
	}
	else if (mantaSliderMode == SliderModeThree)
	{
		manta_set_LED_slider(SliderOne,sequencer[currentSequencer].step[hexagon].octave + 1); // OCTAVE add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,sequencer[currentSequencer].step[hexagon].length); // the step length is already between 1-8
	}
	else 
	{
		//Should not get here.
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
			prev_recent_hex = most_recent_hex;
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
			prev_keyboard_hex = most_recent_upper_hex;
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
	
	if (new_lower_hex) processTouchLowerHex(most_recent_hex);

	if (new_upper_hex) processTouchUpperHex(most_recent_upper_hex);
		
	if (new_func_button) processTouchFunctionButton(most_recent_func_button);

}

void processTouchLowerHex(uint8_t hexagon)
{
	// if we are in edit mode, then we want to be able to touch the hexagons to edit the sequencer[currentSequencer] stages, without changing which stages will be stepped on
	if (edit_vs_play == EDITMODE)
	{
		if (prev_recent_hex != hexagon)
		{
			// LEDS
			if (current_step != prev_recent_hex)
			{
				manta_set_LED_hex(prev_recent_hex, RedOff);
			}
			
			if (sequencer[currentSequencer].step[prev_recent_hex].toggled)
			{
				manta_set_LED_hex(prev_recent_hex, AmberOn);
			}
			
			if (range_vs_toggle_mode == RANGEMODE)
			{
				manta_set_LED_hex(prev_recent_hex, AmberOff);
				manta_set_LED_hex(hexagon, AmberOn);
			}
			
			if (key_vs_option == KEYMODE)
			{
				setKeyboardLEDsFor(hexagon);
			}
			setSliderLEDsFor(hexagon);
			
			manta_set_LED_hex(hexagon, AmberOff);
			manta_set_LED_hex(hexagon, RedOn);
		}
	}
	else
	{
		//we are in "play" mode and we want our touches to control which steps the sequencer can step on
		if (arp_vs_seq == SEQMODE) // note ons should toggle sequencer steps in and out of the pattern
		{
			if (sequencer[currentSequencer].notestack.toggle(&sequencer[currentSequencer].notestack,hexagon))
			{
				sequencer[currentSequencer].step[hexagon].toggled = 1;
				manta_set_LED_hex(hexagon, AmberOn);
			}
			else
			{
				sequencer[currentSequencer].step[hexagon].toggled = 0;
				manta_set_LED_hex(hexagon, AmberOff);
				if (hexagon == current_step)
				{
					manta_set_LED_hex(hexagon, RedOff);
				}
			}
				
			if (key_vs_option == KEYMODE)
			{
				setKeyboardLEDsFor(hexagon);
			}
			setSliderLEDsFor(hexagon);
		}
		else  // "arp mode", note ons should add to pattern, note offs should remove from pattern, so pattern only sounds when fingers are down (not sure if this is useful)
		{
			// need to create a "note off" message in order to do this properly
		}
		
	}
	
	manta_send_LED();
	new_lower_hex = 0;
	
}

void processTouchUpperHex(uint8_t hexagon)
{
	if (key_vs_option == KEYMODE)
	{
		most_recent_pitch = keyboard_pattern[hexagon-32];
		
		if (most_recent_pitch == 255)
		{
			if (sequencer[currentSequencer].step[most_recent_hex].note == 0)
			{
				sequencer[currentSequencer].step[most_recent_hex].note = 1;
			} 
			else
			{
				sequencer[currentSequencer].step[most_recent_hex].note = 0;
			}		
			
		} 
		else if (most_recent_pitch == 254)
		{
			if(sequencer[currentSequencer].step[most_recent_hex].note) 
			{
				// down an octave
				if (--current_seq_octave < 0)
				{
					current_seq_octave = 0;
					//make a rest
					//sequencer[currentSequencer].step[most_recent_hex].note = 0;
				}
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, current_seq_octave+1);
				}
				sequencer[currentSequencer].step[most_recent_hex].octave = current_seq_octave;
			}
		}
		else if (most_recent_pitch == 253)
		{
			if(sequencer[currentSequencer].step[most_recent_hex].note)
			{
				//up an octave
				if (++current_seq_octave > 7)
				{
					current_seq_octave = 7;
				}
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, current_seq_octave+1);
				}
				sequencer[currentSequencer].step[most_recent_hex].octave = current_seq_octave;
			}
		}
		else
		{
			sequencer[currentSequencer].step[most_recent_hex].note = 1;
			sequencer[currentSequencer].step[most_recent_hex].pitch = most_recent_pitch;
			//sequencer[currentSequencer].step[most_recent_hex][2] = most_recent_pitch + (current_seq_octave * 12);

			sequencer[currentSequencer].step[most_recent_hex].octave = current_seq_octave;
			sequencer[currentSequencer].step[most_recent_hex].hexagon = most_recent_upper_hex;

			manta_set_LED_hex(hexagon, Red);
		}
		

		if (current_step == most_recent_hex)
		{
			//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
			uint32_t DACtemp = (uint32_t)sequencer[currentSequencer].step[current_step].pitch;
			DACtemp += (sequencer[currentSequencer].step[current_step].octave * 12);
			DACtemp *= 546125;
			DACtemp /= 1000;
			DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
		}
		
		setKeyboardLEDsFor(most_recent_hex);
		
		//set memory variables
		new_upper_hex = 0;
		prev_recent_pitch = most_recent_pitch;
	}
	else
	{
		//otherwise the upper hexagons are being used to set the alternative options
		uint8_t whichHex = hexagon - 32;
		if (option_pattern[whichHex] == 2 && whichHex < NUM_PATTERNS)
		{
			sequencer[currentSequencer].setPattern(&sequencer[currentSequencer],whichHex);
			
			prev_pattern_hex = most_recent_pattern_hex;
			most_recent_pattern_hex = whichHex;
			
			setModeLEDsFor(currentSequencer);
		}
		else if ((option_pattern[whichHex] == 1) && ((whichHex-8) < NUM_GLOBAL_OPTIONS))
		{
			prev_option_hex = most_recent_option_hex;
			most_recent_option_hex = whichHex;
			
			manta_set_LED_hex(32 + prev_option_hex, Amber);
			manta_set_LED_hex(32 + most_recent_option_hex, Red);
		}
		else if ((option_pattern[whichHex] == 3) && ((whichHex-14) < NUM_PANEL_MOVES))
		{
			if ((whichHex - 14) != currentSequencer)
			{
				currentSequencer = whichHex - 14;
			
				prev_panel_hex = most_recent_panel_hex;
				most_recent_panel_hex = whichHex;
			
				setSequencerLEDsFor(currentSequencer);
				setModeLEDsFor(currentSequencer);
				
				prev_step = sequencer[currentSequencer].prevStep;
				current_step = sequencer[currentSequencer].currentStep;
			}
		}

	}
	
	manta_send_LED();
}

void processTouchFunctionButton(MantaButton button)
{
	//make this toggle three different possibilities
	if (button == ButtonTopLeft)
	{
		if (mantaSliderMode == SliderModeOne)
		{
			mantaSliderMode = SliderModeTwo;
			manta_set_LED_button(ButtonTopLeft, Amber);
		}
		else if (mantaSliderMode == SliderModeTwo)
		{
			mantaSliderMode = SliderModeThree;
			manta_set_LED_button(ButtonTopLeft,Red);
		}
		else if (mantaSliderMode == SliderModeThree)
		{
			mantaSliderMode = SliderModeOne;
			manta_set_LED_button(ButtonTopLeft, Off);
		}
		else
		{
			//Should not get here.
		}
		setSliderLEDsFor(most_recent_hex);
	}
	else if (button == ButtonTopRight)
	{
		if (edit_vs_play == EDITMODE)
		{
			edit_vs_play = PLAYMODE;
			
			if (most_recent_hex != current_step)
			{
				manta_set_LED_hex(most_recent_hex, RedOff);
			}
			if (sequencer[currentSequencer].step[most_recent_hex].toggled)
			{
				manta_set_LED_hex(most_recent_hex, AmberOn);
			}
				
			manta_set_LED_button(ButtonTopRight, Amber);
		}
		else
		{
			edit_vs_play = EDITMODE;
			
			if (most_recent_hex != current_step)
			{
				manta_set_LED_hex(most_recent_hex, AmberOff);
				manta_set_LED_hex(most_recent_hex, RedOn);
			}
			else
			{
				manta_set_LED_hex(most_recent_hex, RedOff);
				manta_set_LED_hex(most_recent_hex, AmberOn);
			}
			
			manta_set_LED_button(ButtonTopRight, Off);
			
		}
		
		if (key_vs_option == KEYMODE)
		{
			setKeyboardLEDsFor(most_recent_hex);
		}
			
		setSliderLEDsFor(most_recent_hex);
	}
	else if (button == ButtonBottomLeft)
	{
		if (key_vs_option == KEYMODE)
		{
			key_vs_option = OPTIONMODE;
			setModeLEDsFor(currentSequencer);
			manta_set_LED_button(ButtonBottomLeft, Red);
		}
		else
		{
			key_vs_option = KEYMODE;
			setKeyboardLEDsFor(most_recent_hex);
			manta_set_LED_button(ButtonBottomLeft, Off);
		}
		
	}
	else if (button == ButtonBottomRight)
	{
		if (arp_vs_seq == ARPMODE)
		{
			arp_vs_seq = SEQMODE;
			manta_set_LED_button(ButtonBottomRight, Off);
		}
		else
		{
			arp_vs_seq = ARPMODE;
			manta_set_LED_button(ButtonBottomRight, Red);
		}
	}
	else
	{
		// Should never happen.
	}
	
	// Actually send LED data to Manta.
	manta_send_LED();
	
	new_func_button = 0;
}

void processSliderSequencer(uint8_t sliderNum, uint16_t val)
{
	if (mantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)	
		{		
			sequencer[currentSequencer].step[most_recent_hex].cv1 = val;
		}
		else if (sliderNum == SliderTwo)	
		{
			sequencer[currentSequencer].step[most_recent_hex].cv2 = val;
		}
		else 
		{
			// Should never happen.
		}
	
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (current_step == most_recent_hex)
		{
			dacsend(sliderNum, 0, val);
		}
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
		if (sliderNum == SliderOne)
		{
			sequencer[currentSequencer].step[most_recent_hex].cv3 = val;
		}
		else if (sliderNum == SliderTwo)
		{
			sequencer[currentSequencer].step[most_recent_hex].cv4 = val;
		}
		else
		{
			// Should never happen.
		}
		
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (current_step == most_recent_hex)
		{
			// This might be wrong?
			dacsend((sliderNum + 2), 0, val);
		}
	}
	else if (mantaSliderMode == SliderModeThree)
	{
		 // otherwise, you're in third slider mode, where top slider is octave and bottom slider is note length
		if (sliderNum == SliderOne)
		{
			uint16_t prevOct = sequencer[currentSequencer].step[most_recent_hex].octave;
			uint16_t newOct = (val >> 9);
			sequencer[currentSequencer].step[most_recent_hex].octave = newOct;
			current_seq_octave = newOct;
			manta_set_LED_slider(0, newOct + 1); // add one to the slider values because a zero turns them off
			manta_send_LED();
			if ((current_step == most_recent_hex) && (prevOct != newOct))
			{
				uint32_t DACtemp = (uint32_t)sequencer[currentSequencer].step[current_step].pitch;
				DACtemp += (sequencer[currentSequencer].step[current_step].octave * 12);
				DACtemp *= 546125;
				DACtemp /= 1000;
				DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
		}
		else if (sliderNum == SliderTwo)
		{
			sequencer[currentSequencer].step[most_recent_hex].length = (val >> 9) + 1; //step length (should be 1-8)
			manta_set_LED_slider(1, (val >> 9) + 1); // add one to the slider values because a zero turns them off
			manta_send_LED();
		}
		else
		{
			// Should not get here.
		}
	}
	else
	{
		// Should not get here.
	}
}

// UTILITIES
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



