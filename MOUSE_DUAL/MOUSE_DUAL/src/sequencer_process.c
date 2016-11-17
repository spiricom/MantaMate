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


#define BLINKER_FRAMES 25
int blinkerFrameCounter = 0;
uint8_t blinkerState = 0;

// Typedef versions of Manta modes.

//------------------  S T R U C T U R E S  -------------------
typedef enum MantaSequencer {
	SequencerOne = 0,
	SequencerTwo,
	SequencerBoth,
	SequencerNil,
} MantaSequencer;


	
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

typedef enum KeyboardOptionMode
{
	KeyboardMode,
	OptionMode,
	KeyboardOptionModeNil,
}KeyboardOptionMode;

typedef enum PanelSwitch
{
	PanelLeft,
	PanelRight,
	PanelSwitchNil,
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
void processReleaseLowerHex(uint8_t hexagon);
void processTouchUpperHex(uint8_t hexagon);

// LEDs
void setSliderLEDsFor(MantaSequencer seq, uint8_t note);
void setKeyboardLEDsFor(MantaSequencer seq, uint8_t note, uint8_t setRed);
void setModeLEDsFor(MantaSequencer seq);
void setSequencerLEDsFor(MantaSequencer seq /*, TriggerPanel panel*/);
void uiStep(MantaSequencer seq);

// UTILITIES
void seqwait(void);
uint8_t hexUIToStep(uint8_t hexagon);
uint8_t stepToHexUI(MantaSequencer seq, uint8_t noteIn);

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

tSequencer trigSequencer[NUM_SEQ];

tSequencer *sequencer; 

int editNoteOn; 
tNoteStack editStack;

MantaSliderMode mantaSliderMode = SliderModeOne;
MantaEditPlayMode edit_vs_play = EditMode;
MantaButton currentFunctionButton = ButtonTopLeft;
GlobalOptionType full_vs_split = FullMode;
GlobalOptionType pitch_vs_trigger = PitchMode;

#define RANGEMODE 0
#define TOGGLEMODE 1
#define SINGLEMODE 0
#define DUALMODE 1
#define SEQMODE 0
#define ARPMODE 1

uint8_t range_top = 15;
uint8_t range_bottom = 0;
uint8_t range_vs_toggle_mode = TOGGLEMODE;
uint8_t order_vs_pattern = PATTERN;

KeyboardOptionMode key_vs_option = KeyboardMode;
uint8_t arp_vs_seq = SEQMODE;

/* - - - - - - - - MantaState (touch events + history) - - - */
MantaSequencer currentSequencer = SequencerOne; // current Sequencer is CURRENTLY EDITING SEQUENCER
TriggerPanel currentPanel = PanelOne;

// Flags for new inputs.
uint8_t new_upper_hex = 0;
uint8_t new_lower_hex = 0;
uint8_t new_release_lower_hex = 0;
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

uint8_t prevUpperHexUI = 210;
uint8_t currentUpperHexUI = 0;


int8_t current_seq_octave = 3;

extern uint8_t func_button_states[4];

uint8_t numSeqUI = 1; // 2 if Split mode

#define sequencerGet(SEQ,STEP,PARAM)		SEQ.get(&SEQ,STEP,PARAM)
#define sequencerSet(SEQ,STEP,PARAM,VAL)	SEQ.get(&SEQ,STEP,PARAM,VAL)

#define toggleSequencerStep(SEQ,STEP)			sequencer[SEQ].toggle(&sequencer[SEQ], STEP)


void initSequencer(void)
{
	initTimers();
	
	tNoteStackInit(&editStack,32);
	editNoteOn = -1;
	
	currentSequencer = SequencerOne;
	currentPanel = PanelOne;
	
	for (int i = 0; i < NUM_SEQ; i++)
	{		
		tSequencerInit(&fullSequencer[i],32);
		tSequencerInit(&splitSequencer[i],16);
		tSequencerInit(&trigSequencer[i],32);
	}
	
	sequencer = trigSequencer;
	
	setSequencerLEDsFor(currentSequencer /*, PanelOne*/);
	
	if (full_vs_split == SplitMode)
	{
		// Selects and sets LEDs for OTHER sequencer.
		setSequencerLEDsFor((currentSequencer+1) % NUM_SEQ);  
	}

	if (pitch_vs_trigger == PitchMode)
	{
		setKeyboardLEDsFor(currentSequencer, 0, 0);
	}
	else
	{
		setModeLEDsFor(currentSequencer);
	}
	
	
	; // now write that data to the manta 
}



void sequencerStep(void)
{

	LED_Toggle(LED5); 
	
	//check if you are supposed to increment yet (based on the length value of the current step)

	int offset,lengthToMatch,cstep,curr;
	int shouldStep = 0; 
	
	for (int i = 0; i < NUM_SEQ; i++)
	{
		offset = i * 2;
		
		cstep = sequencer[i].currentStep;
		
		// Can probably clean up the logic below...
		
		sequencer[i].lengthCounter += 1;
		lengthToMatch = sequencer[i].step[cstep].length;
		
		if (sequencer[i].lengthCounter >= lengthToMatch)
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
}

void processSequencer(void)
{
	// this is the function to take input from the Manta and figure out what to do with it.
	// the manta data is in a global array called butt_states[i]
	// look at processKeys() for an example of how to find changes in the data. This function will get called even when nothing is different about the data the manta is sending - it hasn't parsed it yet to know whether there is a significant change (i.e a new hexagon button press)
	int i = 0;
	uint8_t newHexUI = 0;
	uint8_t newUpperHexUI = 0;

	
	//check the sequencer step hexagons
	for (i = 0; i < MAX_STEPS; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			newHexUI = i;
			new_lower_hex = 1;
		}
		
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newHexUI = i;
			new_release_lower_hex = 1;
		}
		pastbutt_states[i] = butt_states[i];
	}
	
	//check the upper keyboard selector notes
	for (i = MAX_STEPS; i < 48; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			//an upper hexagon was just pressed
			newUpperHexUI = i;
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
			currentFunctionButton = i;
			new_func_button = 1;
		}
		past_func_button_states[i] = func_button_states[i];
	}
	
	if (new_release_lower_hex) processReleaseLowerHex(newHexUI);
	
	if (new_lower_hex) processTouchLowerHex(newHexUI);

	if (new_upper_hex) processTouchUpperHex(newUpperHexUI);
	
	if (new_func_button) processTouchFunctionButton(currentFunctionButton);

}
#define EDIT_STACK 0
void processReleaseLowerHex(uint8_t hexagon)
{
	if (edit_vs_play == EditMode)
	{
		if (editNoteOn == hexagon)
		{
			editNoteOn = -1;
		}
	}
	
		
	new_release_lower_hex = 0;
}

void clearEditStackHexes(MantaSequencer seq)
{
	// Otherwise, clear the stack and add the current hex.
	
	int size = editStack.size;
	int note = 0;
	
	for (int i = 0; i < size; i++)
	{
		note = editStack.notestack[i];
		manta_set_LED_hex(note, Off);
		
		if (sequencer[seq].step[note].toggled)
		{
			manta_set_LED_hex(note, AmberOn);
		}
	}
	
	editStack.clear(&editStack);
	
	editStack.add(&editStack,currentHexUI);
	
	if (edit_vs_play == EditMode)
	{
		manta_set_LED_hex(currentHexUI, Red);
	}
	else
	{
		manta_set_LED_hex(currentHexUI, AmberOn);
	}
	
}

void processTouchLowerHex(uint8_t hexagon)
{
	blinkerState = 0;
	blinkerFrameCounter = 0;
	
	// Set hexUIs for this processing frame.
	prevHexUI = currentHexUI;
	currentHexUI = hexagon;
	
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
	
	if (key_vs_option == KeyboardMode)
	{
		if (editStack.size > 1)
		{
			setKeyboardLEDsFor(currentSequencer, hexUIToStep(hexagon), 0);
		}
		else
		{
			setKeyboardLEDsFor(currentSequencer, hexUIToStep(hexagon), 1);
		}	
	}
	else
	{
		setModeLEDsFor(currentSequencer);
	}
	
	if (editStack.size > 1)
	{
		setSliderLEDsFor(currentSequencer, -1);
	}
	else
	{
		setSliderLEDsFor(currentSequencer, hexagon);
	}
	
	uint8_t step = hexUIToStep(hexagon);
	uint8_t uiHexCurrentStep = stepToHexUI(currentSequencer, sequencer[currentSequencer].currentStep);
	uint8_t uiHexPrevStep = stepToHexUI(currentSequencer, sequencer[currentSequencer].prevStep);
	
	
	// if we are in edit mode, then we want to be able to touch the hexagons to edit the sequencer[currentSequencer] stages, without changing which stages will be stepped on
	if (edit_vs_play == EditMode)
	{
		if (editNoteOn >= 0)
		{
			// If the first hex added is still touched, add new hex to edit stack.
			if (editStack.contains(&editStack,currentHexUI) < 0)
			{
				editStack.add(&editStack, currentHexUI);
				manta_set_LED_hex(currentHexUI,Red);
			}
		}
		else
		{
			clearEditStackHexes(prevSequencer);
			
			editNoteOn = editStack.first(&editStack);
			
			manta_set_LED_hex(editNoteOn,Red);
		}

		// Below not really doing anything atm.
		if (prevHexUI != hexagon)
		{
			
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
			if (range_vs_toggle_mode == RANGEMODE)
			{
				manta_set_LED_hex(prevHexUI, AmberOff);
				manta_set_LED_hex(hexagon, AmberOn);
			}
			// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		}
	}
	else
	{
		// Toggle mode
		editStack.remove(&editStack, prevHexUI);
		editStack.add(&editStack,currentHexUI);
		
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
		else  
		{
			// "arp mode", note ons should add to pattern, note offs should remove from pattern, so pattern only sounds when fingers are down
		}

	}
	
	new_lower_hex = 0;
}


void setParameter(MantaSequencer seq, StepParameterType param, uint16_t value)
{
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].toggled = value;
	else if (param == Length)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].length = value;
	else if (param == CV1)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].cv1 = value;
	else if (param == CV2)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].cv2 = value;
	else if (param == CV3)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].cv3 = value;
	else if (param == CV4)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].cv4 = value;
	else if (param == Pitch)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].pitch = value;
	else if (param == Octave)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].octave = value;
	else if (param == Note)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].note = value;
	else if (param == KbdHex)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].kbdhex = value;
	else if (param == On1)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].on[PanelOne] = value;
	else if (param == On2)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].on[PanelTwo] = value;
	else if (param == On3)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].on[PanelThree] = value;
	else if (param == On4)
	for (; i < size; i++) sequencer[seq].step[editStack.notestack[i]].on[PanelFour] = value;
	else
	;
	
}

void processTouchUpperHex(uint8_t hexagon)
{
	
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;

	if (key_vs_option == KeyboardMode)
	{
		current_pitch = keyboard_pattern[hexagon-MAX_STEPS];
		
		if (current_pitch == 255)
		{
			if (!(sequencer[currentSequencer].step[editStack.first(&editStack)].note))
			{
				setParameter(currentSequencer,Note,1);
			}
			else
			{
				setParameter(currentSequencer,Note,0);
			}
		}
		else if (current_pitch == 254)
		{
			if(sequencer[currentSequencer].step[editStack.first(&editStack)].note)
			{
				// down an octave
				sequencer[currentSequencer].downOctave(&sequencer[currentSequencer]);
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, sequencer[currentSequencer].octave+1);
				}
				setParameter(currentSequencer,Octave,sequencer[currentSequencer].octave);
				//sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;
			}
		}
		else if (current_pitch == 253)
		{
			if(sequencer[currentSequencer].step[editStack.first(&editStack)].note)
			{
				//up an octave
				sequencer[currentSequencer].upOctave(&sequencer[currentSequencer]);
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				if (mantaSliderMode == SliderModeThree)
				{
					manta_set_LED_slider(0, sequencer[currentSequencer].octave+1);
				}
				setParameter(currentSequencer,Octave,sequencer[currentSequencer].octave);
				//sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;
			}
		}
		else
		{
			setParameter(currentSequencer, Note, 1);
			//sequencer[currentSequencer].step[note].note = 1;
			
			setParameter(currentSequencer, Pitch, current_pitch);
			//sequencer[currentSequencer].step[note].pitch = current_pitch;

			setParameter(currentSequencer, Octave, current_seq_octave);
			//sequencer[currentSequencer].step[note].octave = current_seq_octave;
			
			setParameter(currentSequencer, KbdHex, currentUpperHexUI);
			//sequencer[currentSequencer].step[note].kbdhex = currentUpperHexUI;

			manta_set_LED_hex(hexagon, Red);
		}
		
		int cStep = sequencer[currentSequencer].currentStep;
		if (editStack.contains(&editStack,cStep) != -1)
		{
			//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
			uint32_t DACtemp = (uint32_t)sequencer[currentSequencer].step[cStep].pitch;
			DACtemp += (sequencer[currentSequencer].step[cStep].octave * 12);
			DACtemp *= 546125;
			DACtemp /= 1000;
			DAC16Send(0, DACtemp); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
		}
		
		setKeyboardLEDsFor(currentSequencer, editStack.first(&editStack), 1);
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
			
			currentUpperHexUI = 0;
			
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
				if (pitch_vs_trigger == PitchMode)
				{
					if ((whichHex - 14) != currentSequencer)
					{
						currentSequencer = whichHex - 14;
						
						prev_panel_hex = current_panel_hex;
						current_panel_hex = whichHex;
						
						setSequencerLEDsFor(currentSequencer);
					}
				}
				else
				{
					int whichWay = whichHex - 14;
					if (whichWay == PanelLeft)
					{
						if (--currentPanel < 0)
						{
							currentPanel = 0;
						}
					}
					else if (whichWay == PanelRight)
					{
						if (++currentPanel > 7)
						{
							currentPanel = 7;
						}
					}
					
					if (currentPanel < 4)
					{
						currentSequencer = SequencerOne;
					}
					else
					{
						currentSequencer = SequencerTwo;
					}
				}
				
			}
		}
		setModeLEDsFor(currentSequencer);

	}
	
	;
	
	//set memory variables
	new_upper_hex = 0;
	prev_pitch = current_pitch;
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
		
		if (editStack.size > 1)
		{
			setSliderLEDsFor(currentSequencer, -1);
		}
		else
		{
			setSliderLEDsFor(currentSequencer, editStack.first(&editStack));
		}
		
	}
	else if (button == ButtonTopRight)
	{
		if (edit_vs_play == EditMode)
		{
			edit_vs_play = PlayToggleMode;
			
			clearEditStackHexes(currentSequencer);
			
			int currHex = editStack.first(&editStack);
			
			if (currHex != sequencer[currentSequencer].currentStep)
			{
				manta_set_LED_hex(currHex, AmberOn);
			}
			
			if (sequencer[currentSequencer].step[hexUIToStep(currHex)].toggled)
			{
				manta_set_LED_hex(currHex, AmberOn);
			}
			
			manta_set_LED_button(ButtonTopRight, Amber);
		}
		else
		{
			edit_vs_play = EditMode;
			
			int currHex = editStack.first(&editStack);
			if (currHex != sequencer[currentSequencer].currentStep)
			{
				manta_set_LED_hex(currHex, Red);
			}
			else
			{
				manta_set_LED_hex(currHex, Amber);
			}
			
			manta_set_LED_button(ButtonTopRight, Off);
			
		}
		
		if (key_vs_option == KeyboardMode)
		{
			setKeyboardLEDsFor(currentSequencer, editStack.first(&editStack), 1);
		}
		
		setSliderLEDsFor(currentSequencer, currentHexUI);
	}
	else if (button == ButtonBottomLeft)
	{
		if (key_vs_option == KeyboardMode)
		{
			key_vs_option = OptionMode;
			setModeLEDsFor(currentSequencer);
			manta_set_LED_button(ButtonBottomLeft, Red);
		}
		else
		{
			key_vs_option = KeyboardMode;
			setKeyboardLEDsFor(currentSequencer, 0, 0);
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
	;
	
	new_func_button = 0;
}

void processSliderSequencer(uint8_t sliderNum, uint16_t val)
{
	int note = hexUIToStep(currentHexUI);
	
	int currStep = sequencer[currentSequencer].currentStep;
	
	int uiHexCurrentStep = stepToHexUI(currentSequencer,currStep);

	if (mantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)
		{
			setParameter(currentSequencer, CV1, val);
			//sequencer[currentSequencer].step[note].cv1 = val;
			
		}
		else if (sliderNum == SliderTwo)
		{
			setParameter(currentSequencer, CV2, val);
			//sequencer[currentSequencer].step[note].cv2 = val;
		}
		else
		{
			// Should never happen.
		}
		
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off

		if (editStack.contains(&editStack,uiHexCurrentStep) != -1)
		{
			dacsend(sliderNum, 0, val);
		}
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
		if (sliderNum == SliderOne)
		{
			setParameter(currentSequencer, CV3, val);
			//sequencer[currentSequencer].step[note].cv3 = val;
		}
		else if (sliderNum == SliderTwo)
		{
			setParameter(currentSequencer, CV4, val);
			//sequencer[currentSequencer].step[note].cv4 = val;
		}
		else
		{
			// Should never happen.
		}
		
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		
		if (editStack.contains(&editStack,uiHexCurrentStep) != -1)
		{
			dacsend((sliderNum + 2), 0, val);
		}
	}
	else if (mantaSliderMode == SliderModeThree)
	{
		// otherwise, you're in third slider mode, where top slider is octave and bottom slider is note length
		if (sliderNum == SliderOne)
		{
			uint16_t prevOct = 0;
			if (editStack.size <= 1)
			{
				prevOct = sequencer[currentSequencer].step[editStack.first(&editStack)].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameter(currentSequencer,Octave,newOct);
			//sequencer[currentSequencer].step[note].octave = newOct;
			
			current_seq_octave = newOct;
			
			manta_set_LED_slider(0, newOct + 1); // add one to the slider values because a zero turns them off

			if ((editStack.contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
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
			setParameter(currentSequencer,Length, (val >> 9) + 1);
			//sequencer[currentSequencer].step[note].length = (val >> 9) + 1; //step length (should be 1-8)
			manta_set_LED_slider(1, (val >> 9) + 1); // add one to the slider values because a zero turns them off
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

void uiStep(MantaSequencer seq)
{
	int cStep = sequencer[seq].currentStep;
	int pStep = sequencer[seq].prevStep;
	int uiHexCurrentStep = stepToHexUI(seq, cStep);
	int uiHexPrevStep = stepToHexUI(seq, pStep);

	if (pitch_vs_trigger == PitchMode)
	{
		if (edit_vs_play == EditMode)
		{
			
			if (editStack.contains(&editStack,uiHexCurrentStep) >= 0)
			{
				manta_set_LED_hex(uiHexCurrentStep, AmberOn);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOn);
			}
			
			if (editStack.contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, AmberOff);
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
	}
	else if (pitch_vs_trigger == TriggerMode)
	{
		
		// Not implemented properly yet. 
		if (edit_vs_play == EditMode)
		{
			if (editStack.contains(&editStack,uiHexCurrentStep) >= 0)
			{
				manta_set_LED_hex(uiHexCurrentStep, AmberOn);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOn);
			}
			
			if (editStack.contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, AmberOff);
			}
			else
			{
				manta_set_LED_hex(uiHexPrevStep, RedOff);
			}
			
		}
		else
		{
			if (sequencer[seq].step[pStep].on[currentPanel])
			{
				manta_set_LED_hex(uiHexPrevStep, RedOn);
			}
			else
			{
				manta_set_LED_hex(uiHexPrevStep, RedOff);
			}
			
			if (sequencer[seq].step[cStep].on[currentPanel])
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOff);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOn);
			}
		}
	}
	else
	{
		
	}
	
	
	;
}

void setKeyboardLEDsFor(MantaSequencer seq, uint8_t note, uint8_t setRed)
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
		if (setRed)
		{
			manta_set_LED_hex(sequencer[seq].step[note].kbdhex, Red);
		}
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
	uint16_t cv1 = 0;
	uint16_t cv2 = 0;
	uint16_t cv3 = 0;
	uint16_t cv4 = 0;
	uint16_t octave = 0;
	uint16_t length = 0;
	
	if (note >= 0)
	{
		cv1 = (sequencer[seq].step[note].cv1 >> 9) + 1;
		cv2 = (sequencer[seq].step[note].cv2 >> 9) + 1;
		cv3 = (sequencer[seq].step[note].cv3 >> 9) + 1;
		cv4 = (sequencer[seq].step[note].cv4 >> 9) + 1;
		octave = (sequencer[seq].step[note].octave + 1);
		length = (sequencer[seq].step[note].length);
	}

	
	if (mantaSliderMode == SliderModeOne)
	{
		manta_set_LED_slider(SliderOne, cv1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, cv2); // add one to the slider values because a zero turns them off
	}
	else if (mantaSliderMode == SliderModeTwo)
	{
		manta_set_LED_slider(SliderOne, cv3); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, cv4); //add one to the slider values because a zero turns them off/
	}
	else if (mantaSliderMode == SliderModeThree)
	{
		manta_set_LED_slider(SliderOne, octave); // OCTAVE add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, length); // the step length is already between 1-8
	}
	else
	{
		//Should not get here.
	}
	
	
}

void setSequencerLEDsFor(MantaSequencer seq /*, TriggerPanel panel*/)
{
	int hexUI = 0;
	
	for (int i = 0; i < sequencer[seq].maxLength; i++)
	{
		hexUI = stepToHexUI(seq, i);
		
		manta_set_LED_hex(hexUI, Off);
		
	
		if (sequencer[seq].step[i].toggled)
		{
			manta_set_LED_hex(hexUI, AmberOn);
		}

/*
		if (pitch_vs_trigger == TriggerMode)
		{
			if (sequencer[seq].step[i].on[panel])
			{
				manta_set_LED_hex(hexUI, Red);
			}
			else
			{
				manta_set_LED_hex(hexUI, RedOn);
			}
		}
		else
		{
			
		}
		*/
		
	}
	
	if (edit_vs_play == EditMode)
	{
		manta_set_LED_hex(editStack.first(&editStack), RedOn);
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



void blinkersOn(void)
{
	if (edit_vs_play == EditMode)
	{
		manta_set_LED_hex(currentHexUI, RedOn);
	}
	
}

void blinkersOff(void)
{
	if (edit_vs_play == EditMode)
	{
		manta_set_LED_hex(currentHexUI, RedOff);
	}
}

void blinkersToggle(void)
{
	
	if (++blinkerFrameCounter >= BLINKER_FRAMES)
	{
		blinkerFrameCounter = 0;
		blinkerState = !blinkerState;

		if (blinkerState)
		{
			//blinkersOn();
		}
		else
		{
			//blinkersOff();
		}
	}
}

// UTILITIES
uint8_t hexUIToStep(uint8_t hexagon)
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
	
	int step = 0;
	
	if (full_vs_split == SplitMode)
	{
		if (hex < 16)
		{
			step = hex;
		}
		else
		{
			step = hex - 16;
		}
	}
	else
	{
		step = hex;
	}
	
	return step;
}

uint8_t stepToHexUI(MantaSequencer seq, uint8_t stepIn)
{
	int hex = 0;
	int step = 0;
	if (full_vs_split == SplitMode)
	{
		if (stepIn < 0)
		{
			step = 0;
		}
		else if (stepIn < 16)
		{
			step = stepIn;
		}
		else
		{
			step = 15;
		}
		
		if (seq == SequencerOne)
		{
			hex = step;
		}
		else
		{
			hex = step + 16;
		}
		
	}
	else
	{
		if (stepIn < 0)
		{
			hex = 0;
		}
		else if (stepIn < 32)
		{
			hex = stepIn;
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



