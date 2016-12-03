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
#include "utilities.h"
#include "sequencer.h"


#define BLINKER_FRAMES 25
int blinkerFrameCounter = 0;
uint8_t blinkerState = 0;

PanelSwitch panelSwitch[NUM_PANEL_MOVES] =
{
	PanelLeft,
	PanelRight
};


// INPUT
void processTouchFunctionButton(MantaButton button);
void processTouchLowerHex(uint8_t hexagon);
void processReleaseLowerHex(uint8_t hexagon);
void processTouchUpperHex(uint8_t hexagon);
void processReleaseUpperHex(uint8_t hexagon);

// OUTPUT
void dacSendPitchMode(MantaSequencer seq, uint8_t step);
void dacSendTriggerMode(MantaSequencer seq, uint8_t step);

// LEDs
void setPanelSelectLEDs(void);
void setSliderLEDsFor(MantaSequencer seq, int note);
void setKeyboardLEDsFor(MantaSequencer seq, int note);
void setModeLEDsFor(MantaSequencer seq);
void setSequencerLEDsFor(MantaSequencer seq);
void setTriggerPanelLEDsFor(MantaSequencer seq, TriggerPanel panel);
void uiStep(MantaSequencer seq);
void resetSliderMode(void);

// UTILITIES
void seqwait(void);
uint32_t get16BitPitch(MantaSequencer seq, uint8_t step);
void setCurrentSequencer(MantaSequencer seq);
void setParameterForEditStackSteps(MantaSequencer seq, StepParameterType param, uint16_t value);
void setParameterForStep(MantaSequencer seq, uint8_t step, StepParameterType param, uint16_t value);
uint16_t getParameterFromStep(MantaSequencer seq, uint8_t step, StepParameterType param);
void resetEditStack(void);
uint8_t hexUIToStep(uint8_t hexagon);
uint8_t stepToHexUI(MantaSequencer seq, uint8_t noteIn);

/* - - - - - - KEY PATTERNS - - - - - - - -*/

typedef enum TriggerPanelButtonType
{
	S1TrigPanelSelect = 0,
	S2TrigPanelSelect,
	SXTrigPanelNil
	
} TriggerPanelButtonType;
// Upper keyboard pattern
uint8_t keyboard_pattern[16] = {0,2,4,5,7,9,11,12,1,3,255,6,8,10,254,253};
	
uint8_t trigger_pattern[16] = 
{
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	S2TrigPanelSelect,
	S2TrigPanelSelect,
	S2TrigPanelSelect,
	S2TrigPanelSelect,
	
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil
};
	
// Additional options pattern
uint8_t option_pattern[16] = {2,2,2,2,2,2,2,2,1,1,1,1,0,0,3,3};
	
// UpDown pattern


/* - - - - - - - - MODES - - - - - - - - - - */

#define NUM_SEQ 2

tSequencer pitchFullSequencer[NUM_SEQ]; 
tSequencer pitchSplitSequencer[NUM_SEQ];

tSequencer trigFullSequencer[NUM_SEQ];
tSequencer trigSplitSequencer[NUM_SEQ];

tSequencer *sequencer; 

int editNoteOn; 
tNoteStack editStack;

int keyNoteOn;

tNoteStack noteOnStack;
tNoteStack noteOffStack;

#define RANGEMODE 0
#define TOGGLEMODE 1
#define SINGLEMODE 0
#define DUALMODE 1

uint8_t range_top = 15;
uint8_t range_bottom = 0;




/* - - - - - - - - MantaState (touch events + history) - - - */
MantaSequencer currentSequencer = SequencerOne; // current Sequencer is CURRENTLY EDITING SEQUENCER

TriggerPanel currentPanel[2] = 
{
	PanelOne,
	PanelOne
};

// Flags for new inputs.
uint8_t new_upper_hex = 0;
uint8_t new_lower_hex = 0;
uint8_t new_release_lower_hex = 0;
uint8_t new_release_upper_hex = 0;
uint8_t new_func_button = 0;

uint8_t prev_pattern_hex = 0;
uint8_t current_pattern_hex = 0;

uint8_t prev_panel_hex = 0;
uint8_t current_panel_hex = 0;

uint8_t prev_option_hex = 0;
uint8_t current_option_hex = 0;

uint8_t currentHexUIOff = 0;

uint8_t currentHexUI = 0;
uint8_t prevHexUI = 0;

uint8_t currentUpperHexUI = 0;

extern uint8_t func_button_states[4];

uint8_t numSeqUI = 1; // 2 if Split mode

#define sequencerGet(SEQ,STEP,PARAM)		SEQ.get(&SEQ,STEP,PARAM)
#define sequencerSet(SEQ,STEP,PARAM,VAL)	SEQ.get(&SEQ,STEP,PARAM,VAL)

#define toggleSequencerStep(SEQ,STEP)			sequencer[SEQ].toggle(&sequencer[SEQ], STEP)


void setCurrentSequencer(MantaSequencer seq)
{
	if (currentSequencer != seq)
	{
		editNoteOn = -1;
		currentSequencer = seq;
	}
	
}

void initSequencer(void)
{
	initTimers();
	
	// Sequencer Modes
	pattern_type =				LeftRightRowUp;
	currentMantaSliderMode =	SliderModeOne;
	prevMantaSliderMode =		SliderModeOne;
	edit_vs_play =				EditMode;
	currentFunctionButton =		ButtonTopLeft;
	full_vs_split =				FullMode;
	pitch_vs_trigger =			PitchMode;
	range_vs_toggle_mode =		ToggleMode;
	key_vs_option =				KeyboardMode;
	seq_vs_arp =				SeqMode;
	
	
	tNoteStackInit(&editStack,		32);
	tNoteStackInit(&noteOffStack,	32);
	tNoteStackInit(&noteOnStack,	32);
	
	setCurrentSequencer(SequencerOne);
	keyNoteOn = -1;
	
	for (int i = 0; i < NUM_SEQ; i++)
	{		
		tSequencerInit(&pitchFullSequencer[i],	32);
		tSequencerInit(&pitchSplitSequencer[i],	16);
		
		tSequencerInit(&trigFullSequencer[i],	32);
		tSequencerInit(&trigSplitSequencer[i],  16);
	}
	
	if (pitch_vs_trigger == PitchMode)
	{
		sequencer = pitchFullSequencer;
	}
	else //TriggerMode
	{
		sequencer = trigFullSequencer;
	}
	
	setSequencerLEDsFor(currentSequencer);
	
	if (full_vs_split == SplitMode)
	{
		// Selects and sets LEDs for OTHER sequencer.
		setSequencerLEDsFor((currentSequencer+1) % NUM_SEQ);  
	}

	setKeyboardLEDsFor(currentSequencer, -1);
}

void sequencerStep(void)
{
	LED_Toggle(LED5);
	
	int offset,cstep,curr;
	
	for (int i = 0; i < NUM_SEQ; i++)
	{
		offset = i * 2;
		
		cstep = sequencer[i].currentStep;
		
		sequencer[i].lengthCounter += 1;
		
		if (sequencer[i].lengthCounter >= sequencer[i].step[cstep].length)
		{
			sequencer[i].next(&sequencer[i]); // Move to next step, seq 1.
			
			curr = sequencer[i].currentStep;
			
			if (sequencer[i].stepGo)
			{
				if (pitch_vs_trigger == PitchMode)
				{
					dacSendPitchMode(i,curr);
					//dacSendTriggerMode(i,curr);
				}
				else // TriggerMode
				{
					dacSendTriggerMode(i,curr);
				}

				// Start sequencer 1 and 2 timers: tc1, tc2.
				// Timer callbacks tc1_irq and tc2_irq (in main.c) set dac trigger outputs low on every step.
				if (i == SequencerOne)
				{
					tc_start(tc1, TC1_CHANNEL);
				}
				else // SequencerTwo
				{
					tc_start(tc2, TC2_CHANNEL);
				}

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
	uint8_t newHexUIOn = 0;
	uint8_t newHexUIOff = 0;
	uint8_t newUpperHexUI = 0;

	
	//check the sequencer step hexagons
	for (i = 0; i < MAX_STEPS; i++)
	{
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			newHexUIOn = i;
			new_lower_hex = 1;
			noteOnStack.add(&noteOnStack,	i);
		}
		
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newHexUIOff = i;
			new_release_lower_hex = 1;
			noteOffStack.add(&noteOffStack,	i);
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
		
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newUpperHexUI = i;
			new_release_upper_hex = 1;
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
	
	if (new_release_lower_hex) processReleaseLowerHex(newHexUIOff);
	
	if (new_lower_hex) processTouchLowerHex(newHexUIOn);
	
	if (new_release_upper_hex) processReleaseUpperHex(newUpperHexUI);

	if (new_upper_hex) processTouchUpperHex(newUpperHexUI);
	
	if (new_func_button) processTouchFunctionButton(currentFunctionButton);

}

void processReleaseLowerHex(uint8_t hexagon)
{
	if (edit_vs_play == EditMode)
	{
		uint8_t hex = 0;
		for (int i = 0; i < noteOffStack.size; i++)
		{
			hex = noteOffStack.notestack[i];
			
			if (hex == editNoteOn)
			{
				editNoteOn = -1;
			}
		}
	}
	
	noteOffStack.clear(&noteOffStack);
	new_release_lower_hex = 0;
}

void processTouchLowerHex(uint8_t hexagon)
{
	if (editStack.contains(&editStack,hexagon) < 0)
	{
		resetSliderMode();
	}
	
	blinkerState = 0;
	blinkerFrameCounter = 0;
	
	// Set hexUIs for this processing frame.
	prevHexUI = currentHexUI;
	currentHexUI = hexagon;
	
	int sequencerToSet = currentSequencer;
	if (full_vs_split == SplitMode)
	{
		if (hexagon < 16)
		{
			setCurrentSequencer(SequencerTwo);
		}
		else
		{
			setCurrentSequencer(SequencerOne);
		}
		
		if (sequencerToSet != currentSequencer)
		{
			resetEditStack();
			
			if (edit_vs_play != TrigToggleMode)
			{
				setSequencerLEDsFor(SequencerOne);
				setSequencerLEDsFor(SequencerTwo);
			}
			else
			{
				setTriggerPanelLEDsFor(SequencerOne,currentPanel[SequencerOne]);
				setTriggerPanelLEDsFor(SequencerTwo,currentPanel[SequencerTwo]);
			}
		}	
	}
	
	
	uint8_t uiHexCurrentStep = stepToHexUI(currentSequencer, sequencer[currentSequencer].currentStep);
	uint8_t uiHexPrevStep = stepToHexUI(currentSequencer, sequencer[currentSequencer].prevStep);
	int step = 0;
	
	for (int i = 0; i < noteOnStack.size; i++)
	{
		hexagon = noteOnStack.notestack[i];
		step = hexUIToStep(hexagon);
		
		if (edit_vs_play == EditMode)
		{
			if (editNoteOn >= 0)
			{
				// If the first hex added is still touched, add new hex to edit stack.
				// (contains returns index of hex in notestack if hex is found)
				if (editStack.contains(&editStack, hexagon) < 0)
				{
					editStack.add(&editStack, hexagon);
					manta_set_LED_hex(hexagon, Red);
				}
				
			}
			else
			{
				resetEditStack();
				
				setSequencerLEDsFor(sequencerToSet);
				
				editNoteOn = editStack.first(&editStack);
				
				manta_set_LED_hex(editNoteOn,Red);
			}

		}
		else if (edit_vs_play == PlayToggleMode)
		{
			editStack.remove(&editStack, prevHexUI);
			editStack.add(&editStack,currentHexUI);
		
			if (seq_vs_arp == SeqMode) // note ons should toggle sequencer steps in and out of the pattern
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
		else //TrigToggleMode
		{
			editStack.remove(&editStack, prevHexUI);
			editStack.add(&editStack,currentHexUI);	
		
			int currPanel = currentPanel[currentSequencer];
		
			if (sequencer[currentSequencer].step[step].on[currPanel])
			{
				sequencer[currentSequencer].step[step].on[currPanel] = 0;
				manta_set_LED_hex(currentHexUI, RedOff);
			}
			else
			{
				sequencer[currentSequencer].step[step].on[currPanel] = 1;
				manta_set_LED_hex(currentHexUI, RedOn);
			}
		}
	}
	

	if (key_vs_option == KeyboardMode)
	{
		if (edit_vs_play != TrigToggleMode)
		{
			if (editStack.size > 1)
			{
				setKeyboardLEDsFor(currentSequencer, -1);
			}
			else
			{
				setKeyboardLEDsFor(currentSequencer, hexUIToStep(hexagon));
			}
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
		setSliderLEDsFor(currentSequencer, hexUIToStep(hexagon));
	}
	
	noteOnStack.clear(&noteOnStack);
	new_lower_hex = 0;
}


void processReleaseUpperHex(uint8_t hexagon)
{
	if (pitch_vs_trigger == PitchMode)
	{
		if (keyboard_pattern[hexagon-MAX_STEPS] < 253)
		{
			// Exit SliderModePitch
			if (keyNoteOn == hexagon)
			{
				keyNoteOn = -1;
			
				currentMantaSliderMode = prevMantaSliderMode;
				if (editStack.size <= 1)
				{
					setSliderLEDsFor(currentSequencer, editStack.first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentSequencer, -1);
				}
			}
			
		}
		
		if (key_vs_option == KeyboardMode)
		{
			if ((keyboard_pattern[hexagon-MAX_STEPS] == 253) || (keyboard_pattern[hexagon-MAX_STEPS] == 254))
			{
				if (currentMantaSliderMode != SliderModeThree)
				{
					setSliderLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
				}
				currentMantaSliderMode = prevMantaSliderModeForOctaveHexDisable;
			}
		}
	}
	
	new_release_upper_hex = 0;
}

void processTouchUpperHex(uint8_t hexagon)
{
	currentUpperHexUI = hexagon;

	if (key_vs_option == KeyboardMode)
	{
		if (pitch_vs_trigger == PitchMode)
		{
			int upperHexType = keyboard_pattern[hexagon-MAX_STEPS];
				
			if (upperHexType == 255)
			{
				//if (editStack.size <= 1)
				{
					// handle case when more than one note in editStack (check if they all have same parameter value
					// need to be able to set multiple notes to rest too
					int step = hexUIToStep(editStack.first(&editStack));
					if (!(getParameterFromStep(currentSequencer, step, Note)))
					{
						setParameterForEditStackSteps(currentSequencer,Note,1);
						//setParameterForStep(currentSequencer, step, Note, 1);
					}
					else
					{
						setParameterForEditStackSteps(currentSequencer,Note,0);
						//setParameterForStep(currentSequencer, step, Note, 0);
					}
				}
				//else
				{
						
					//setParameterForEditStackSteps(currentSequencer,Note,1);
				}
			}
			else if (upperHexType == 254)
			{
				// down an octave
				sequencer[currentSequencer].downOctave(&sequencer[currentSequencer]);
				
				// Temporarily set slider LEDs to display current octave.
				manta_set_LED_slider(SliderOne, sequencer[currentSequencer].octave+1);
				
				/*
				if (currentMantaSliderMode != SliderModeNil)
				{
					prevMantaSliderModeForOctaveHexDisable = currentMantaSliderMode;
					prevMantaSliderMode = currentMantaSliderMode;
				}
				
				currentMantaSliderMode = SliderModeNil;
				*/
				
				setParameterForEditStackSteps(currentSequencer,Octave,sequencer[currentSequencer].octave);
				//sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;
			}
			else if (upperHexType == 253)
			{
				//up an octave
				sequencer[currentSequencer].upOctave(&sequencer[currentSequencer]);
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				
				// Temporarily set slider LEDs to display current octave.
				manta_set_LED_slider(SliderOne, sequencer[currentSequencer].octave+1);
				
				/*
				if (currentMantaSliderMode != SliderModeNil) 
				{
					prevMantaSliderModeForOctaveHexDisable = currentMantaSliderMode;
				}
				
				currentMantaSliderMode = SliderModeNil;
				*/
				
				setParameterForEditStackSteps(currentSequencer,Octave,sequencer[currentSequencer].octave);
				//sequencer[currentSequencer].step[note].octave = sequencer[currentSequencer].octave;

			}
			else
			{
				if (keyNoteOn == -1)
				{
					keyNoteOn = hexagon;
				
					// In this case, upperHexType is pitch on keyboard.
					setParameterForEditStackSteps(currentSequencer, Note, 1);
					
					setParameterForEditStackSteps(currentSequencer, Pitch, upperHexType);

					setParameterForEditStackSteps(currentSequencer, Octave, sequencer[currentSequencer].octave);
					
					setParameterForEditStackSteps(currentSequencer, KbdHex, currentUpperHexUI);

					manta_set_LED_hex(hexagon, Red);
				
				
					// Enter SliderModePitch 
					prevMantaSliderMode = currentMantaSliderMode;
					currentMantaSliderMode = SliderModePitch;
				
					if (editStack.size <= 1)
					{
						setSliderLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentSequencer, -1);
					}
				}
			}
				
			int cStep = sequencer[currentSequencer].currentStep;
			if (editStack.contains(&editStack,stepToHexUI(currentSequencer,cStep)) != -1)
			{
				//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, cStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
				
			setKeyboardLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
		}
		else // TriggerMode
		{
			int whichUpperHex = currentUpperHexUI - MAX_STEPS;
			int whichTrigPanel = trigger_pattern[whichUpperHex];
			
			MantaSequencer whichSeq = currentSequencer;
			int whichPanel = 0;
			int cont = 0;
			
			if (whichTrigPanel == S1TrigPanelSelect)
			{
				whichSeq = SequencerOne;
				whichPanel = whichUpperHex - 8;
				cont= 1;
			}
			else if (whichTrigPanel == S2TrigPanelSelect)
			{
				whichSeq = SequencerTwo;
				whichPanel = whichUpperHex - 4;
				cont = 1;
			}
			
			if (cont) //dumb method
			{
				if (whichSeq != currentSequencer)
				{
					setCurrentSequencer(whichSeq);
				
					if (edit_vs_play != TrigToggleMode)
					{
						setKeyboardLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
						setSequencerLEDsFor(currentSequencer);
					}
				}
			
				int uiOffset = MAX_STEPS;
			
				if (whichSeq == SequencerOne) uiOffset += 8;
				else /*SequencerTwo*/ uiOffset += 4;
			
				if (edit_vs_play == EditMode)
				{
					if (whichTrigPanel != SXTrigPanelNil)
					{
						int step = hexUIToStep(editStack.first(&editStack));
					
						if (!getParameterFromStep(currentSequencer, step, On1 + whichPanel))
						{
							setParameterForEditStackSteps(currentSequencer, On1 + whichPanel, 1);
						
							manta_set_LED_hex(whichPanel + uiOffset, Red);
						}
						else
						{
							setParameterForEditStackSteps(currentSequencer, On1 + whichPanel, 0);
						
							manta_set_LED_hex(whichPanel + uiOffset, Amber);
						}
					}
				
				
				}
				else if (edit_vs_play == TrigToggleMode)
				{
				
					int prevPanel = currentPanel[currentSequencer];
					currentPanel[currentSequencer] = whichPanel;
				
					setTriggerPanelLEDsFor(currentSequencer,currentPanel[currentSequencer]);
				
					manta_set_LED_hex(prevPanel + uiOffset, Amber);
					manta_set_LED_hex(currentPanel[currentSequencer] + uiOffset, Red);
				}
				else //PlayToggleMode
				{
				
				}
			}
			
		}
			
	}
	else // OptionMode
	{
		uint8_t whichHex = hexagon - MAX_STEPS;
		if (option_pattern[whichHex] == 2 && whichHex < NUM_PATTERNS)
		{
			sequencer[currentSequencer].setPattern(&sequencer[currentSequencer],whichHex);
				
			prev_pattern_hex = current_pattern_hex;
			current_pattern_hex = whichHex;
		}
		else if ((option_pattern[whichHex] == 1) && ((whichHex-8) < NUM_GLOBAL_OPTIONS))
		{
			resetEditStack();
			
			prev_option_hex = current_option_hex;
			current_option_hex = whichHex;
				
			int whichMode = whichHex - 8;
				
			currentHexUI = 0;
				
			currentUpperHexUI = 0;
				
			setCurrentSequencer(SequencerOne);
			
			if (whichMode == FullMode)
			{
				full_vs_split = FullMode;
					
				if (pitch_vs_trigger == PitchMode)
				{
					sequencer = pitchFullSequencer;
				}
				else //TriggerMode
				{
					sequencer = trigFullSequencer;
				}
					
				setSequencerLEDsFor(SequencerOne);
					
			}
			else if (whichMode == SplitMode)
			{
				full_vs_split = SplitMode;
					
				if (pitch_vs_trigger == PitchMode)
				{
					sequencer = pitchSplitSequencer;
				}
				else
				{
					sequencer = trigSplitSequencer;
				}
					
				setSequencerLEDsFor(SequencerOne);
				setSequencerLEDsFor(SequencerTwo);
			}
			else if (whichMode == PitchMode)
			{
				pitch_vs_trigger = PitchMode;
					
				if (full_vs_split == FullMode)
				{
					sequencer = pitchFullSequencer;
					setCurrentSequencer(SequencerOne);
					setSequencerLEDsFor(SequencerOne);
				}
				else //SplitMode
				{
					sequencer = pitchSplitSequencer;
					setSequencerLEDsFor(SequencerOne);
					setSequencerLEDsFor(SequencerTwo);
				}	
					
						
			} 
			else if (whichMode == TriggerMode)
			{
				pitch_vs_trigger = TriggerMode;
					
				if (full_vs_split == FullMode)
				{
					sequencer = trigFullSequencer;
						
					setCurrentSequencer(SequencerOne);
					setSequencerLEDsFor(SequencerOne);
				}
				else //SplitMode
				{
					// Implement SplitMode.
					sequencer = trigSplitSequencer;
					
					setSequencerLEDsFor(SequencerOne);
					setSequencerLEDsFor(SequencerTwo);
				}
			}
			else
			{
				// No other mode, sorry.
			}
				
			setModeLEDsFor(SequencerOne);
				
				
				
		}
		else if ((option_pattern[whichHex] == 3) && ((whichHex-14) < NUM_PANEL_MOVES))
		{
			if (pitch_vs_trigger == PitchMode)
			{
				if (full_vs_split == FullMode)
				{
					if ((whichHex - 14) != currentSequencer)
					{
						setCurrentSequencer(whichHex - 14);
					
						prev_panel_hex = current_panel_hex;
						current_panel_hex = whichHex;
					
						setSequencerLEDsFor(currentSequencer);
					}
				}
			}
			else // TriggerMode 
			{
				if (edit_vs_play == PlayToggleMode && full_vs_split == FullMode)
				{
					if ((whichHex - 14) != currentSequencer)
					{
						setCurrentSequencer(whichHex - 14);
						
						prev_panel_hex = current_panel_hex;
						current_panel_hex = whichHex;
						
						setSequencerLEDsFor(currentSequencer);
					}
				}
			}
		}
		setModeLEDsFor(currentSequencer);

	}

	//set memory variables
	new_upper_hex = 0;
}

void resetSliderMode(void)
{
	if (keyNoteOn >= 0)
	{
		keyNoteOn = -1;
		currentMantaSliderMode = prevMantaSliderMode;
		setSliderLEDsFor(currentSequencer, hexUIToStep(current_pattern_hex));
	}
}

void processTouchFunctionButton(MantaButton button)
{
	resetSliderMode();
	
	if (button == ButtonTopLeft)
	{
		//toggle three different SliderModes in PitchMode
		if (pitch_vs_trigger == PitchMode)
		{
			if (currentMantaSliderMode == SliderModeOne)
			{
				currentMantaSliderMode = SliderModeTwo;
				manta_set_LED_button(ButtonTopLeft, Amber);
			}
			else if (currentMantaSliderMode == SliderModeTwo)
			{
				currentMantaSliderMode = SliderModeThree;
				manta_set_LED_button(ButtonTopLeft,Red);
			}
			else //SliderModeThree
			{
				currentMantaSliderMode = SliderModeOne;
				manta_set_LED_button(ButtonTopLeft, Off);
			}
			
			if (editStack.size > 1)
			{
				setSliderLEDsFor(currentSequencer, -1);
			}
			else
			{
				setSliderLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
			}
		}
	}
	else if (button == ButtonTopRight)
	{
		resetEditStack();
		
		if (full_vs_split == SplitMode)
		{
			setSequencerLEDsFor(SequencerOne);
			setSequencerLEDsFor(SequencerTwo);
		}
		else
		{
			setSequencerLEDsFor(currentSequencer);
		}
		
		
		if (pitch_vs_trigger == PitchMode)
		{
			if (edit_vs_play == EditMode)
			{
				edit_vs_play = PlayToggleMode;
				
				manta_set_LED_button(ButtonTopRight, Amber);
			}
			else // PlayToggleMode
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
				setKeyboardLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
			}
			
			if (full_vs_split == FullMode)
			{
				setSequencerLEDsFor(currentSequencer);
			}
			else
			{
				setSequencerLEDsFor(SequencerOne);
				setSequencerLEDsFor(SequencerTwo);
			}
			
		} 
		else //TriggerMode
		{
			if (edit_vs_play == EditMode)
			{
				// Switch to OptionMode.
				prev_key_vs_option = key_vs_option;
				key_vs_option = OptionMode;
				manta_set_LED_button(ButtonBottomLeft, Red);
				
				edit_vs_play = PlayToggleMode;
				manta_set_LED_button(ButtonTopRight, Amber);
			}
			else if (edit_vs_play == PlayToggleMode)
			{
				// Switch back to other KeyboardOptionMode.
				key_vs_option = prev_key_vs_option;
				if (key_vs_option == KeyboardMode)
				{
					manta_set_LED_button(ButtonBottomLeft, Off);
				}
				else
				{
					manta_set_LED_button(ButtonBottomLeft, Red);
				}
				
				edit_vs_play = TrigToggleMode;		
				manta_set_LED_button(ButtonTopRight, Red);
			}
			else //TrigToggleMode
			{
				edit_vs_play = EditMode;
				manta_set_LED_button(ButtonTopRight, Off);
			}
			
			if (edit_vs_play == TrigToggleMode)
			{
				if (full_vs_split == SplitMode)
				{
					setTriggerPanelLEDsFor(SequencerOne,currentPanel[SequencerOne]);
					setTriggerPanelLEDsFor(SequencerTwo,currentPanel[SequencerTwo]);
				}
				else //FullMode
				{
					setTriggerPanelLEDsFor(currentSequencer,currentPanel[currentSequencer]);
				}
			}
			else
			{
				if (full_vs_split == SplitMode)
				{
					setSequencerLEDsFor(SequencerOne);
					setSequencerLEDsFor(SequencerTwo);
				}
				else //FullMode
				{
					setSequencerLEDsFor(currentSequencer);
				}
				
			}	
			
			if (edit_vs_play == EditMode)
			{
				setKeyboardLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
			}
			else if (edit_vs_play == TrigToggleMode)
			{
				setKeyboardLEDsFor(SequencerOne, -1);
				setPanelSelectLEDs();
			}
			else // PlayToggleMode
			{
				setModeLEDsFor(currentSequencer);
			}
		}
		
		setSliderLEDsFor(currentSequencer, hexUIToStep(editStack.first(&editStack)));
	}
	else if (button == ButtonBottomLeft)
	{
		if (key_vs_option == KeyboardMode)
		{
			key_vs_option = OptionMode;
			setModeLEDsFor(currentSequencer);
			manta_set_LED_button(ButtonBottomLeft, Red);
		}
		else //OptionMode
		{
			key_vs_option = KeyboardMode;
				
			if (edit_vs_play == TrigToggleMode)
			{
				setKeyboardLEDsFor(currentSequencer, 0);
			}
			else if (edit_vs_play == EditMode)
			{
				setKeyboardLEDsFor(currentSequencer, -1);
			}
			else //PlayToggleMode
			{
				 if (pitch_vs_trigger != TriggerMode)
				 {
					 setKeyboardLEDsFor(currentSequencer, -1);
				 }
			}

			manta_set_LED_button(ButtonBottomLeft, Off);
		}
		
	}
	else if (button == ButtonBottomRight)
	{
		if (seq_vs_arp == ArpMode)
		{
			seq_vs_arp = SeqMode;
			manta_set_LED_button(ButtonBottomRight, Off);
		}
		else //SeqMode
		{
			seq_vs_arp = ArpMode;
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

	if (currentMantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)
		{
			setParameterForEditStackSteps(currentSequencer, CV1, val);
			//sequencer[currentSequencer].step[note].cv1 = val;
			
		}
		else // SliderTwo
		{
			setParameterForEditStackSteps(currentSequencer, CV2, val);
			//sequencer[currentSequencer].step[note].cv2 = val;
		}
		
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off

		if (editStack.contains(&editStack,uiHexCurrentStep) != -1)
		{
			dacsend(currentSequencer * 2 + sliderNum, 0, val);
		}
	}
	else if (currentMantaSliderMode == SliderModeTwo)
	{
		// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
		if (sliderNum == SliderOne)
		{
			setParameterForEditStackSteps(currentSequencer, CV3, val);
			//sequencer[currentSequencer].step[note].cv3 = val;
		}
		else // SliderTwo
		{
			setParameterForEditStackSteps(currentSequencer, CV4, val);
			//sequencer[currentSequencer].step[note].cv4 = val;
		}

		
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		
		if (editStack.contains(&editStack,uiHexCurrentStep) != -1)
		{
			dacsend(currentSequencer * 2 + sliderNum, 1, val);
		}
	}
	else if (currentMantaSliderMode == SliderModeThree)
	{
		// otherwise, you're in third slider mode, where top slider is octave and bottom slider is note length
		if (sliderNum == SliderOne)
		{
			uint16_t prevOct = 0;
			if (editStack.size <= 1)
			{
				prevOct = sequencer[currentSequencer].step[hexUIToStep(editStack.first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentSequencer,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((editStack.contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
			{
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, currStep)); 
			}
		}
		else //SliderTwo
		{
			setParameterForEditStackSteps(currentSequencer,Length, (val >> 9) + 1);
			//sequencer[currentSequencer].step[note].length = (val >> 9) + 1; //step length (should be 1-8)
			manta_set_LED_slider(SliderTwo, (val >> 9) + 1); // add one to the slider values because a zero turns them off
		}
	}
	else if (currentMantaSliderMode == SliderModePitch)
	{
		if (sliderNum == SliderOne)
		{   
			//Octave
			uint16_t prevOct = 0;
			if (editStack.size <= 1)
			{
				prevOct = sequencer[currentSequencer].step[hexUIToStep(editStack.first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentSequencer,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((editStack.contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
			{
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
		}
		else // SliderTwo
		{
			// Fine Tune
			uint16_t prevFine = 0;
			if (editStack.size <= 1)
			{
				prevFine = sequencer[currentSequencer].step[hexUIToStep(editStack.first(&editStack))].fine;
			}
			uint16_t newFine = val;
			
			setParameterForEditStackSteps(currentSequencer,Fine,val);
			
			manta_set_LED_slider(SliderTwo, (val >> 9) + 1); // add one to the slider values because a zero turns them off

			if ((editStack.contains(&editStack,uiHexCurrentStep) != -1) && (prevFine != newFine))
			{
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
			
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

	if (edit_vs_play == EditMode)
	{
		if (pitch_vs_trigger == PitchMode)
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
		else //TriggerMode
		{
			if (editStack.contains(&editStack,uiHexCurrentStep) >= 0)
			{
				manta_set_LED_hex(uiHexCurrentStep, AmberOn);
			}
			else
			{
				if (sequencer[seq].step[cStep].on[currentPanel[currentSequencer]])
				{
					manta_set_LED_hex(uiHexCurrentStep, Red);
				}
				else
				{
					manta_set_LED_hex(uiHexCurrentStep, RedOn);
				}
			}
			
			if (editStack.contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, AmberOff);
			}
			else
			{
				if (sequencer[seq].step[pStep].on[currentPanel[currentSequencer]])
				{
					manta_set_LED_hex(uiHexPrevStep, Amber);
				}
				else
				{
					manta_set_LED_hex(uiHexPrevStep, RedOff);
				}
			}
		}
		
			
		
			
	}
	else if (edit_vs_play == PlayToggleMode)
	{
		if (pitch_vs_trigger == TriggerMode)
		{
			if (sequencer[seq].step[pStep].toggled)
			{
				manta_set_LED_hex(uiHexPrevStep, Amber);
			}

			if (!sequencer[seq].step[cStep].on[currentPanel[currentSequencer]])
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOn);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, Red);		
			}
		}
		else
		{
			manta_set_LED_hex(uiHexPrevStep, RedOff);
			manta_set_LED_hex(uiHexCurrentStep, RedOn);
		}
		
	}
	else // TrigToggleMode
	{
		manta_set_LED_hex(uiHexPrevStep, AmberOff);
		manta_set_LED_hex(uiHexCurrentStep, AmberOn);
	}
}

void setPanelSelectLEDs(void)
{
	manta_set_LED_hex(currentPanel[SequencerOne] + MAX_STEPS + 8, Red);
	manta_set_LED_hex(currentPanel[SequencerTwo] + MAX_STEPS + 4, Red);	
}

void setKeyboardLEDsFor(MantaSequencer seq, int note)
{	
	int setRed = 1;
	int nt = 0;
	
	if (note < 0)
	{
		setRed = 0;
	}
	else
	{
		nt = note;
	}
	
	if (pitch_vs_trigger == PitchMode)
	{
		if (sequencer[seq].step[nt].note == 1)
		{
			for (int j = 0; j < 16; j++)
			{
				if (keyboard_pattern[j] < 200)
				{
					manta_set_LED_hex(j+MAX_STEPS, Amber);
				}
				else
				{
					if ((keyboard_pattern[j] == 253) || (keyboard_pattern[j] == 254))
					{
						manta_set_LED_hex(j+MAX_STEPS, Off);
					}
					else
					{
						manta_set_LED_hex(j+MAX_STEPS, Off);
					}
					
				}
			}
			
			if (setRed)
			{
				manta_set_LED_hex(sequencer[seq].step[nt].kbdhex, Red);
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
	else // TriggerMode
	{
		for (int i = 0; i < 16; i++)
		{
			int which = trigger_pattern[i];
			if ((which == S1TrigPanelSelect) || (which == S2TrigPanelSelect))
			{
				manta_set_LED_hex(i + MAX_STEPS, Amber);
			}
			else // SXTrigPanelNil
			{
				manta_set_LED_hex(i + MAX_STEPS, Off);
			}
			
		}

		int hexUIOffset = MAX_STEPS;
		
		if (seq == SequencerOne)
		{
			hexUIOffset += 8;
		}
		else //SequencerTwo
		{
			hexUIOffset += 4;
		}
		
		if (setRed)
		{
			for (int panel = 0; panel < 4; panel++)
			{
				if (sequencer[seq].step[note].on[panel])
				{
					manta_set_LED_hex(hexUIOffset + panel, Red);
				}
				else
				{
					manta_set_LED_hex(hexUIOffset + panel, Amber);
				}
			}
		}		
	}
}


void setSliderLEDsFor(MantaSequencer seq, int note)
{
	// CV OUTS
	uint16_t cv1 = 0;
	uint16_t cv2 = 0;
	uint16_t cv3 = 0;
	uint16_t cv4 = 0;
	uint16_t octave = 0;
	uint16_t fine = 0;
	uint16_t length = 0;
	
	if (note >= 0)
	{
		cv1 = (sequencer[seq].step[note].cv1 >> 9) + 1;
		cv2 = (sequencer[seq].step[note].cv2 >> 9) + 1;
		cv3 = (sequencer[seq].step[note].cv3 >> 9) + 1;
		cv4 = (sequencer[seq].step[note].cv4 >> 9) + 1;
		fine = (sequencer[seq].step[note].fine >> 9) + 1;
		octave = (sequencer[seq].step[note].octave + 1);
		length = (sequencer[seq].step[note].length);
	}

	
	if (currentMantaSliderMode == SliderModeOne)
	{
		manta_set_LED_slider(SliderOne, cv1); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, cv2); // add one to the slider values because a zero turns them off
	}
	else if (currentMantaSliderMode == SliderModeTwo)
	{
		manta_set_LED_slider(SliderOne, cv3); // add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, cv4); //add one to the slider values because a zero turns them off/
	}
	else if (currentMantaSliderMode == SliderModeThree)
	{
		manta_set_LED_slider(SliderOne, octave); // OCTAVE add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, length); // the step length is already between 1-8
	}
	else if (currentMantaSliderMode == SliderModePitch)
	{
		manta_set_LED_slider(SliderOne, octave); // OCTAVE add one to the slider values because a zero turns them off
		manta_set_LED_slider(SliderTwo, fine); // the step length is already between 1-8
	}
	else
	{
		//Should not get here.
	}
	
	
}

void setTriggerPanelLEDsFor(MantaSequencer seq, TriggerPanel panel)
{
	int hexUI = 0;
	for (int i = 0; i < sequencer[seq].maxLength; i++)
	{
		hexUI = stepToHexUI(seq,i);
		
		if (sequencer[seq].step[i].on[panel])
		{
			manta_set_LED_hex(hexUI,Red);
		}
		else
		{
			manta_set_LED_hex(hexUI,Off);
		}
	}
}

void setSequencerLEDsFor(MantaSequencer seq)
{
	int hexUI = 0;

	for (int i = 0; i < sequencer[seq].maxLength; i++)
	{
		hexUI = stepToHexUI(seq, i);
		
		if (sequencer[seq].step[i].toggled)
		{
			manta_set_LED_hex(hexUI, Amber);
		}
		else
		{
			manta_set_LED_hex(hexUI, Off);
		}	
	}
	
	int size = editStack.size;
	
	if (edit_vs_play == EditMode)
	{
		for (int i = 0; i < size; i++)
		{
			manta_set_LED_hex(editStack.notestack[i], Red);
		}
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
			if (full_vs_split == FullMode && edit_vs_play != TrigToggleMode)
			{
				if ((i - 14) == seq) // (i-14) is index in range [0,2)
				{
					manta_set_LED_hex(i+MAX_STEPS,Amber);
				}
				else 
				{
					manta_set_LED_hex(i+MAX_STEPS,Red);
				}

			}
			else
			{
				manta_set_LED_hex(i+MAX_STEPS, Off);
			}	
		}
	}
	
	if (pitch_vs_trigger == PitchMode)
	{
		manta_set_LED_hex(MAX_STEPS + 10, Red); 
	}
	else //TriggerMode
	{		
		manta_set_LED_hex(MAX_STEPS + 11, Red);
	}
	
	if (full_vs_split == FullMode)
	{
		manta_set_LED_hex(MAX_STEPS + 8, Red);
	}
	else //SplitMode
	{
		manta_set_LED_hex(MAX_STEPS + 9, Red);
	}
}

// Output
uint32_t get16BitPitch(MantaSequencer seq, uint8_t step)
{
	// take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
	uint32_t DACtemp = ((uint32_t)sequencer[seq].step[step].pitch);
	DACtemp += (sequencer[seq].step[step].octave * 12);
	DACtemp *= 546125;
	DACtemp /= 1000;

	DACtemp += (sequencer[seq].step[step].fine >> 2) - 512;
	
	return DACtemp;
}

void dacSendPitchMode(MantaSequencer seq, uint8_t step)
{
	int offset = 2;
	if (seq ==SequencerOne)
	{
		offset = 0;
	}
	
	if (sequencer[seq].step[step].note)
	{
		// CV1, CV2, CV3, CV4
		dacsend(offset+0, 0, sequencer[seq].step[step].cv1);
		dacsend(offset+1, 0, sequencer[seq].step[step].cv2);
		dacsend(offset+0, 1, sequencer[seq].step[step].cv3);
		dacsend(offset+1, 1, sequencer[seq].step[step].cv4);
		
		// Pitch, Trigger
		DAC16Send(offset+0, get16BitPitch(seq,step));
		DAC16Send(offset+1, 65535);
	}
}

void dacSendTriggerMode(MantaSequencer seq, uint8_t step)
{
	int offset = 2;
	if (seq == SequencerOne)
	{
		offset = 0;
	}
	
	// CV1 , CV2
	dacsend(offset+0, 0, sequencer[seq].step[step].cv1);
	dacsend(offset+1, 0, sequencer[seq].step[step].cv2);
	
	// Trigger 1, Trigger 2, Trigger 3, Trigger 4
	dacsend(offset+0, 1, sequencer[seq].step[step].on[0] * 4095);
	dacsend(offset+1, 1, sequencer[seq].step[step].on[1] * 4095);
	DAC16Send(offset+0,  sequencer[seq].step[step].on[2] * 65535);
	DAC16Send(offset+1,  sequencer[seq].step[step].on[3] * 65535);
}

// UTILITIES
uint16_t getParameterFromStep(MantaSequencer seq, uint8_t step, StepParameterType param)
{
	if (param == Toggled)
	return sequencer[seq].step[step].toggled;
	else if (param == Length)
	return sequencer[seq].step[step].length;
	else if (param == CV1)
	return sequencer[seq].step[step].cv1;
	else if (param == CV2)
	return sequencer[seq].step[step].cv2;
	else if (param == CV3)
	return sequencer[seq].step[step].cv3;
	else if (param == CV4)
	return sequencer[seq].step[step].cv4;
	else if (param == Pitch)
	return sequencer[seq].step[step].pitch;
	else if (param == Fine)
	return sequencer[seq].step[step].fine;
	else if (param == Octave)
	return sequencer[seq].step[step].octave;
	else if (param == Note)
	return sequencer[seq].step[step].note;
	else if (param == KbdHex)
	return sequencer[seq].step[step].kbdhex;
	else if (param == On1)
	return sequencer[seq].step[step].on[PanelOne];
	else if (param == On2)
	return sequencer[seq].step[step].on[PanelTwo];
	else if (param == On3)
	return sequencer[seq].step[step].on[PanelThree];
	else if (param == On4)
	return sequencer[seq].step[step].on[PanelFour];
	else
	return;
}

void setParameterForStep(MantaSequencer seq, uint8_t step, StepParameterType param, uint16_t value)
{
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)
	sequencer[seq].step[step].toggled = value;
	else if (param == Length)
	sequencer[seq].step[step].length = value;
	else if (param == CV1)
	sequencer[seq].step[step].cv1 = value;
	else if (param == CV2)
	sequencer[seq].step[step].cv2 = value;
	else if (param == CV3)
	sequencer[seq].step[step].cv3 = value;
	else if (param == CV4)
	sequencer[seq].step[step].cv4 = value;
	else if (param == Pitch)
	sequencer[seq].step[step].pitch = value;
	else if (param == Fine)
	sequencer[seq].step[step].fine = value;
	else if (param == Octave)
	sequencer[seq].step[step].octave = value;
	else if (param == Note)
	sequencer[seq].step[step].note = value;
	else if (param == KbdHex)
	sequencer[seq].step[step].kbdhex = value;
	else if (param == On1)
	sequencer[seq].step[step].on[PanelOne] = value;
	else if (param == On2)
	sequencer[seq].step[step].on[PanelTwo] = value;
	else if (param == On3)
	sequencer[seq].step[step].on[PanelThree] = value;
	else if (param == On4)
	sequencer[seq].step[step].on[PanelFour] = value;
	else
	;
}

void setParameterForEditStackSteps(MantaSequencer seq, StepParameterType param, uint16_t value)
{
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].toggled = value;
	else if (param == Length)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].length = value;
	else if (param == CV1)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv1 = value;
	else if (param == CV2)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv2 = value;
	else if (param == CV3)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv3 = value;
	else if (param == CV4)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv4 = value;
	else if (param == Pitch)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].pitch = value;
	else if (param == Fine)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].fine = value;
	else if (param == Octave)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = value;
	else if (param == Note)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].note = value;
	else if (param == KbdHex)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].kbdhex = value;
	else if (param == On1)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelOne] = value;
	else if (param == On2)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelTwo] = value;
	else if (param == On3)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelThree] = value;
	else if (param == On4)
	for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelFour] = value;
	else
	;
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
			hex = step + 16;
		}
		else
		{
			hex = step;
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

// Resets edit stack so it only contains most recently touched hex.
// Sets shift/noteon hex to -1 to prevent notes from being added to edit stack until new note is pressed.
void resetEditStack(void)
{
	editStack.clear(&editStack);
	editNoteOn = -1;
	editStack.add(&editStack,currentHexUI);
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



