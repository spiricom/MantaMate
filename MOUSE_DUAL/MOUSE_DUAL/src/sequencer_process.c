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
	SequencerBoth,
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



// Sequencer patterns to be used in the program
#define NUM_PATTERNS 8


#define NUM_GLOBAL_OPTIONS 4
typedef enum GlobalOptionType
{
	FullMode = 0,
	SplitMode,
	PitchMode,
	TriggerMode,
} GlobalOptionType;



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




// INPUT
void processTouchFunctionButton(MantaButton button);
void processTouchLowerHex(uint8_t hexagon);
void processTouchUpperHex(uint8_t hexagon);

// LEDs
void setSliderLEDsFor(MantaSequencer seq, uint8_t note);
void setKeyboardLEDsFor(MantaSequencer seq, uint8_t note);
void setModeLEDsFor(MantaSequencer seq);
void setSequencerLEDsFor(MantaSequencer seq);
void uiStep(MantaSequencer seq);

// UTILITIES
void seqwait(void);
uint8_t uiHexToNote(uint8_t hexagon);
uint8_t noteToHexUI(MantaSequencer seq, uint8_t noteIn);

/* - - - - - - KEY PATTERNS - - - - - - - -*/
// Upper keyboard pattern
uint8_t keyboard_pattern[16] = {0,2,4,5,7,9,11,12,1,3,255,6,8,10,254,253};
	
// Additional options pattern
uint8_t option_pattern[16] = {2,2,2,2,2,2,2,2,1,1,1,1,0,0,3,3};
	
// UpDown pattern


/* - - - - - - - - MODES - - - - - - - - - - */

#define NUM_SEQ 2

tSequencer fullSequencer[NUM_SEQ]; 
tSequencer splitSequencer[NUM_SEQ];

tSequencer *sequencer; 

MantaSliderMode mantaSliderMode = SliderModeOne;
MantaEditPlayMode edit_vs_play = EditMode;
MantaButton current_func_button = ButtonTopLeft;
GlobalOptionType full_vs_split = FullMode;
GlobalOptionType pitch_vs_trigger = PitchMode;

#define RANGEMODE 0
#define TOGGLEMODE 1
#define SINGLEMODE 0
#define DUALMODE 1
#define KEYMODE 0
#define OPTIONMODE 1
#define SEQMODE 0
#define ARPMODE 1

uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = TOGGLEMODE;
uint8_t order_vs_pattern = PATTERN;

uint8_t single_vs_dual = SINGLEMODE;
uint8_t key_vs_option = KEYMODE;
uint8_t arp_vs_seq = SEQMODE;

/* - - - - - - - - MantaState (touch events + history) - - - */
MantaSequencer currentSequencer = SequencerOne; // current Sequencer is CURRENTLY EDITING SEQUENCER

// Flags for new inputs.
uint8_t new_upper_hex = 0;
uint8_t new_lower_hex = 0;
uint8_t new_func_button = 0;

uint8_t prev_pattern_hex = 0;
uint8_t current_pattern_hex = 0;

uint8_t prev_panel_hex = 0;
uint8_t current_panel_hex = 0;

uint8_t prev_option_hex = 0;
uint8_t current_option_hex = 0;

uint8_t currentHexUI = 0;
uint8_t prevHexUI = 0;
	
uint8_t	current_pitch = 0;
uint8_t	prev_pitch = 0;

uint8_t prev_upper_hex = 210; // some garbage
uint8_t current_upper_hex = 0;

int8_t current_seq_octave = 3;

extern uint8_t func_button_states[4];

uint8_t numSeqUI = 1; // 2 if Split mode

#define sequencerGet(SEQ,STEP,PARAM)		SEQ.get(&SEQ,STEP,PARAM)
#define sequencerSet(SEQ,STEP,PARAM,VAL)	SEQ.get(&SEQ,STEP,PARAM,VAL)

#define toggleSequencerStep(SEQ,STEP)			sequencer[SEQ].toggle(&sequencer[SEQ], STEP)


void initSequencer(void)
{
	initTimers();
	
	currentSequencer = SequencerOne;
	
	for (int i = 0; i < NUM_SEQ; i++)
	{		
		tSequencerInit(&fullSequencer[i],32);
		tSequencerInit(&splitSequencer[i],16);
	}
	
	sequencer = fullSequencer;
	
	setSequencerLEDsFor(currentSequencer);
	if (full_vs_split == SplitMode)
	{
		// Selects and sets LEDs for OTHER sequencer.
		setSequencerLEDsFor((currentSequencer+1) % NUM_SEQ);  
	}

	setKeyboardLEDsFor(currentSequencer,0);
	
	manta_send_LED(); // now write that data to the manta 
}

void uiStep(MantaSequencer seq)
{
	int uiHexCurrentStep = noteToHexUI(seq, sequencer[seq].currentStep);
	int uiHexPrevStep = noteToHexUI(seq, sequencer[seq].prevStep);

	if (edit_vs_play == EditMode)
	{
		if (uiHexCurrentStep == currentHexUI)
		{
			manta_set_LED_hex(uiHexCurrentStep, RedOff);
			manta_set_LED_hex(uiHexCurrentStep, AmberOn);
		}
		else
		{
			manta_set_LED_hex(uiHexCurrentStep, RedOn);
		}
		
		if (uiHexPrevStep == currentHexUI)
		{
			manta_set_LED_hex(uiHexPrevStep, AmberOff);
			manta_set_LED_hex(uiHexPrevStep, RedOn);
		}
		else
		{
			manta_set_LED_hex(uiHexPrevStep, RedOff);
		}
	}
	else
	{
		manta_set_LED_hex(uiHexPrevStep, RedOff);
		manta_set_LED_hex(uiHexCurrentStep, RedOn);
	}
	
	manta_send_LED();
}

int lencounter = 0;
int lenToMatch = 0;
int cstep, curr, numnotes;
void sequencerStep(void)
{

	LED_Toggle(LED5); // turn on the red mantamate panel light (should this be LED5?)
	
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
				
			if (sequencer[i].stepGo)
			{
				
				
				// if this step is not a rest, change the pitch  (should the cv1 and cv2 output changes also be gated by note/rest settings?)
				if (sequencer[i].step[curr].note)
				{
					
					dacsend(offset+0, 0, sequencer[i].step[curr].cv1);
					dacsend(offset+1, 0, sequencer[i].step[curr].cv2);
					dacsend(offset+0, 1, sequencer[i].step[curr].cv3);
					dacsend(offset+1, 1, sequencer[i].step[curr].cv4);
					
					uint32_t DACtemp = ((uint32_t)sequencer[i].step[curr].pitch);
					DACtemp += (sequencer[i].step[curr].octave * 12);
					DACtemp *= 546125;
					DACtemp /= 1000;
					DAC16Send(offset+0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
					//DAC16Send(offset+2, ((uint32_t)sequencer[i].step[curr].cv4 * 16)); // tagged CV4 on the 16bit outputs, since I want to use the 12 bits for the gate
					
					
					DAC16Send(offset+1, 65535);
					
					
				}
				
				if (i == 0)
				{
					tc_start(tc1, TC1_CHANNEL);
				}
				else
				{
					tc_start(tc2, TC2_CHANNEL);
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
	uiStep(currentSequencer);
	if (full_vs_split == SplitMode)
	{
		uiStep((currentSequencer+1) % NUM_SEQ);
	}
	manta_send_LED(); // now write that data to the manta
}

void setKeyboardLEDsFor(MantaSequencer seq, uint8_t note)
{
	if (sequencer[seq].step[note].note == 1)
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+MAX_STEPS, Amber);
			}
			else
			{
				manta_set_LED_hex(j+MAX_STEPS, Off);
			}
		}
		manta_set_LED_hex(sequencer[seq].step[note].hexagon, Red);
	}
	else
	{
		for (int j = 0; j < 16; j++)
		{
			if (keyboard_pattern[j] < 200)
			{
				manta_set_LED_hex(j+MAX_STEPS, Red);
			} 
			else if (keyboard_pattern[j] == 255)
			{
				manta_set_LED_hex(j+MAX_STEPS,Amber);
			}
			else
			{
				manta_set_LED_hex(j+MAX_STEPS, Off);
			}
		}
	}

}


void setSliderLEDsFor(MantaSequencer seq, uint8_t note)
{

	// CV OUTS
	if (mantaSliderMode == SliderModeOne)
	{
		manta_set_LED_slider(SliderOne, (sequencer[seq].step[note].cv1 >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,(sequencer[seq].step[note].cv2 >> 9) + 1); // add one to the slider values because a zero turns them off
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		manta_set_LED_slider(SliderOne,(sequencer[seq].step[note].cv3 >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,(sequencer[seq].step[note].cv4 >> 9) + 1); //add one to the slider values because a zero turns them off/
	}
	else if (mantaSliderMode == SliderModeThree)
	{
		manta_set_LED_slider(SliderOne,sequencer[seq].step[note].octave + 1); // OCTAVE add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo,sequencer[seq].step[note].length); // the step length is already between 1-8
	}
	else
	{
		//Should not get here.
	}
}

void setSequencerLEDsFor(MantaSequencer seq)
{
	int hexUI = 0;
	
	for (int i = 0; i < sequencer[seq].maxLength; i++)
	{
		hexUI = noteToHexUI(seq, i);
		
		manta_set_LED_hex(hexUI, Off);
		
		if (sequencer[seq].step[i].toggled == 1)
		{
			manta_set_LED_hex(hexUI, AmberOn);
		}
	}
	
	if (edit_vs_play == EditMode)
	{
		manta_set_LED_hex(currentHexUI, RedOn);
	}
	
	return;
}

void setModeLEDsFor(MantaSequencer seq)
{
	//change the keyboard LEDs to be the MODE leds
	for (int i = 0; i < 16; i++)
	{
		if (option_pattern[i] == 0)
		{
			manta_set_LED_hex(i+MAX_STEPS, Off);
		}
		else if (option_pattern[i] == 1)
		{
			manta_set_LED_hex(i+MAX_STEPS, Amber);
			
		}
		else if (option_pattern[i] == 2 )
		{
			if (i == sequencer[seq].pattern)
			{
				manta_set_LED_hex(i+MAX_STEPS, Red);
			}
			else
			{
				manta_set_LED_hex(i+MAX_STEPS, Amber);
			}
		}
		else if (option_pattern[i] == 3)
		{
			if ((i-14) == seq && (full_vs_split == FullMode))
			{
				manta_set_LED_hex(i+MAX_STEPS,Amber);
			}
			else if (full_vs_split == FullMode)
			{
				manta_set_LED_hex(i+MAX_STEPS,Red);
			}
			else
			{
				manta_set_LED_hex(i+MAX_STEPS, Off);
			}
		}
	}
}


void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it. 
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
	int i = 0;
	
	//check the sequencer step hexagons
	for (i = 0; i < MAX_STEPS; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			prevHexUI = currentHexUI;
			currentHexUI = i;
			new_lower_hex = 1;
		}
		pastbutt_states[i] = butt_states[i];
	}
	
	//check the upper keyboard selector notes
	for (i = MAX_STEPS; i < 48; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			//an upper hexagon was just pressed
			prev_upper_hex = current_upper_hex;
			current_upper_hex = i;
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
			current_func_button = i;
			new_func_button = 1;
		}
		past_func_button_states[i] = func_button_states[i];
	}
	
	if (new_lower_hex) processTouchLowerHex(currentHexUI);

	if (new_upper_hex) processTouchUpperHex(current_upper_hex);
		
	if (new_func_button) processTouchFunctionButton(current_func_button);

}

void processTouchLowerHex(uint8_t hexagon)
{
	int prevSequencer = currentSequencer;
	if (full_vs_split == SplitMode)
	{
		if (hexagon < 16)
		{
			currentSequencer = SequencerOne;
		}
		else
		{
			currentSequencer = SequencerTwo;
		}
		
	}
	
	if (key_vs_option == KEYMODE)
	{
		if (full_vs_split == SplitMode)
		{
			setKeyboardLEDsFor(currentSequencer, uiHexToNote(hexagon));
		}
		else
		{
			setKeyboardLEDsFor(currentSequencer, hexagon);
		}	
	}
	else
	{
		setModeLEDsFor(currentSequencer);
	}
	setSliderLEDsFor(currentSequencer, hexagon);
	
	uint8_t step = uiHexToNote(hexagon);
	uint8_t uiHexCurrentStep = noteToHexUI(currentSequencer, sequencer[currentSequencer].currentStep);
	uint8_t uiHexPrevStep = noteToHexUI(currentSequencer, sequencer[currentSequencer].prevStep);
	
	
	// if we are in edit mode, then we want to be able to touch the hexagons to edit the sequencer[currentSequencer] stages, without changing which stages will be stepped on
	if (edit_vs_play == EditMode)
	{
		if (prevHexUI != hexagon)
		{
			// LEDS
			if (uiHexCurrentStep != prevHexUI)
			{
				manta_set_LED_hex(prevHexUI, RedOff);
			}
			
			if (sequencer[prevSequencer].step[uiHexToNote(prevHexUI)].toggled)
			{
				manta_set_LED_hex(prevHexUI, AmberOn);
			}
			
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if (range_vs_toggle_mode == RANGEMODE)
			{
				manta_set_LED_hex(prevHexUI, AmberOff);
				manta_set_LED_hex(hexagon, AmberOn);
			}
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			
			manta_set_LED_hex(hexagon, AmberOff);
			manta_set_LED_hex(hexagon, RedOn);
		}
	}
	else
	{
		//we are in "play" mode and we want our touches to control which steps the sequencer can step on
		if (arp_vs_seq == SEQMODE) // note ons should toggle sequencer steps in and out of the pattern
		{
			
			
			if (sequencer[currentSequencer].toggle(&sequencer[currentSequencer], step))
			{
				manta_set_LED_hex(hexagon, AmberOn);
			}
			else
			{
				manta_set_LED_hex(hexagon, AmberOff);
				if (hexagon == uiHexCurrentStep)
				{
					manta_set_LED_hex(hexagon, RedOff);
				}
			}
				
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
	int note = uiHexToNote(currentHexUI);
	
	if (key_vs_option == KEYMODE)
	{
		current_pitch = keyboard_pattern[hexagon-MAX_STEPS];
		
		if (current_pitch == 255)
		{
			if (sequencer[currentSequencer].step[note].note == 0)
			{
				sequencer[currentSequencer].step[note].note = 1;
			} 
			else
			{
				sequencer[currentSequencer].step[note].note = 0;
			}		
			
		} 
		else if (current_pitch == 254)
		{
			if(sequencer[currentSequencer].step[note].note) 
			{
				// down an octave
				sequencer[currentSequencer].downOctave(&sequencer[currentSequencer]);
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, sequencer[currentSequencer].octave+1);
				}
				sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;
			}
		}
		else if (current_pitch == 253)
		{
			if(sequencer[currentSequencer].step[note].note)
			{
				//up an octave
				sequencer[currentSequencer].upOctave(&sequencer[currentSequencer]);
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, sequencer[currentSequencer].octave+1);
				}
				sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;
			}
		}
		else
		{
			sequencer[currentSequencer].step[note].note = 1;
			sequencer[currentSequencer].step[note].pitch = current_pitch;
			//sequencer[currentSequencer].step[currentHexUI][2] = current_pitch + (current_seq_octave * 12);

			sequencer[currentSequencer].step[note].octave = current_seq_octave;
			sequencer[currentSequencer].step[note].hexagon = current_upper_hex;

			manta_set_LED_hex(hexagon, Red);
		}
		

		if (sequencer[currentSequencer].currentStep == currentHexUI)
		{
			//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
			uint32_t DACtemp = (uint32_t)sequencer[currentSequencer].step[sequencer[currentSequencer].currentStep].pitch;
			DACtemp += (sequencer[currentSequencer].step[sequencer[currentSequencer].currentStep].octave * 12);
			DACtemp *= 546125;
			DACtemp /= 1000;
			DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
		}
		
		setKeyboardLEDsFor(currentSequencer, note);
		
		//set memory variables
		new_upper_hex = 0;
		prev_pitch = current_pitch;
	}
	else
	{
		//otherwise the upper hexagons are being used to set the alternative options
		uint8_t whichHex = hexagon - MAX_STEPS;
		if (option_pattern[whichHex] == 2 && whichHex < NUM_PATTERNS)
		{
			sequencer[currentSequencer].setPattern(&sequencer[currentSequencer],whichHex);
			
			prev_pattern_hex = current_pattern_hex;
			current_pattern_hex = whichHex;
		}
		else if ((option_pattern[whichHex] == 1) && ((whichHex-8) < NUM_GLOBAL_OPTIONS))
		{
			prev_option_hex = current_option_hex;
			current_option_hex = whichHex;
			
			full_vs_split = whichHex - 8;
			
			currentHexUI = 0;
			
			current_upper_hex = 0;
			
			currentSequencer = SequencerOne;
			
			if (full_vs_split == FullMode)
			{
				sequencer = fullSequencer;
					
				setSequencerLEDsFor(SequencerOne);
				
			}
			else if (full_vs_split == SplitMode)
			{
				sequencer = splitSequencer;	 
					
				setSequencerLEDsFor(SequencerOne);
				setSequencerLEDsFor(SequencerTwo);
			} 
			else 
			{
				// No other sequencer. 
			}	
			
			setModeLEDsFor(SequencerOne);
			
			
	
		}
		else if ((option_pattern[whichHex] == 3) && ((whichHex-14) < NUM_PANEL_MOVES))
		{	
			if (full_vs_split == FullMode)
			{
				if ((whichHex - 14) != currentSequencer)
				{
					currentSequencer = whichHex - 14;
					
					prev_panel_hex = current_panel_hex;
					current_panel_hex = whichHex;
					
					setSequencerLEDsFor(currentSequencer);
				}
			}
		}
		setModeLEDsFor(currentSequencer);

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
		setSliderLEDsFor(currentSequencer, currentHexUI);
	}
	else if (button == ButtonTopRight)
	{
		if (edit_vs_play == EditMode)
		{
			edit_vs_play = PlayToggleMode;
			
			if (currentHexUI != sequencer[currentSequencer].currentStep)
			{
				manta_set_LED_hex(currentHexUI, RedOff);
			}
			
			if (sequencer[currentSequencer].step[uiHexToNote(currentHexUI)].toggled)
			{
				manta_set_LED_hex(currentHexUI, AmberOn);
			}
				
			manta_set_LED_button(ButtonTopRight, Amber);
		}
		else
		{
			edit_vs_play = EditMode;
			
			if (currentHexUI != sequencer[currentSequencer].currentStep)
			{
				manta_set_LED_hex(currentHexUI, AmberOff);
				manta_set_LED_hex(currentHexUI, RedOn);
			}
			else
			{
				manta_set_LED_hex(currentHexUI, RedOff);
				manta_set_LED_hex(currentHexUI, AmberOn);
			}
			
			manta_set_LED_button(ButtonTopRight, Off);
			
		}
		
		if (key_vs_option == KEYMODE)
		{
			setKeyboardLEDsFor(currentSequencer, currentHexUI);
		}
			
		setSliderLEDsFor(currentSequencer, currentHexUI);
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
			setKeyboardLEDsFor(currentSequencer, currentHexUI);
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
	int note = uiHexToNote(currentHexUI);
	
	int currStep = sequencer[currentSequencer].currentStep;
	
	int uiHexCurrentStep = noteToHexUI(currentSequencer,currStep);

	if (mantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)	
		{		
			sequencer[currentSequencer].step[note].cv1 = val;
		}
		else if (sliderNum == SliderTwo)	
		{
			sequencer[currentSequencer].step[note].cv2 = val;
		}
		else 
		{
			// Should never happen.
		}
	
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (uiHexCurrentStep == currentHexUI)
		{
			dacsend(sliderNum, 0, val);
		}
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
		if (sliderNum == SliderOne)
		{
			sequencer[currentSequencer].step[note].cv3 = val;
		}
		else if (sliderNum == SliderTwo)
		{
			sequencer[currentSequencer].step[note].cv4 = val;
		}
		else
		{
			// Should never happen.
		}
		
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		manta_send_LED();
		if (uiHexCurrentStep == currentHexUI)
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
			uint16_t prevOct = sequencer[currentSequencer].step[note].octave;
			uint16_t newOct = (val >> 9);
			sequencer[currentSequencer].step[note].octave = newOct;
			current_seq_octave = newOct;
			manta_set_LED_slider(0, newOct + 1); // add one to the slider values because a zero turns them off
			manta_send_LED();

			if ((uiHexCurrentStep == currentHexUI) && (prevOct != newOct))
			{
				uint32_t DACtemp = (uint32_t)sequencer[currentSequencer].step[currStep].pitch;
				DACtemp += (sequencer[currentSequencer].step[currStep].octave * 12);
				DACtemp *= 546125;
				DACtemp /= 1000;
				DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
		}
		else if (sliderNum == SliderTwo)
		{
			sequencer[currentSequencer].step[note].length = (val >> 9) + 1; //step length (should be 1-8)
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

uint8_t uiHexToNote(uint8_t hexagon)
{
	int hex = 0;
	if (hexagon < 0)
	{
		hex = 0;
	}
	else if (hexagon < 32)
	{
		hex = hexagon;
	}
	else
	{
		hex = 31;
	}
	
	int note = 0;
	
	if (full_vs_split == SplitMode)
	{
		if (hex < 16)
		{
			note = hex;
		}
		else
		{
			note = hex - 16;
		}
	}
	else
	{
		note = hex;
	}
	
	return note;
}

uint8_t noteToHexUI(MantaSequencer seq, uint8_t noteIn)
{
	int hex = 0;
	int note = 0;
	if (full_vs_split == SplitMode)
	{
		if (noteIn < 0)
		{
			note = 0;
		}
		else if (noteIn < 16)
		{
			note = noteIn;
		}
		else
		{
			note = 15;
		}
		
		if (seq == SequencerOne)
		{
			hex = note;
		}
		else
		{
			hex = note + 16;
		}
		
	}
	else
	{
		if (noteIn < 0)
		{
			hex = 0;
		}
		else if (noteIn < 32)
		{
			hex = noteIn;
		}
		else
		{
			hex = 31;
		}
	}
	
	return hex;
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



