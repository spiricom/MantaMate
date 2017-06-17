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
//#include "7Segment.h"

#define NUM_COMP 14

bool compositionMap[2][NUM_COMP];

int currentComp[2] = {-1,-1};

CompositionAction compositionAction = CompositionRead;

int copyStage = 0;
MantaSequencer copyWhichSeq = SequencerNil;
int copyWhichComp = -1;

PanelSwitch panelSwitch[NUM_PANEL_MOVES] =
{
	PanelLeft,
	PanelRight
};

// INPUT
void touchLowerHex			(uint8_t hexagon);
void releaseLowerHex	(uint8_t hexagon);
void touchUpperHex		(uint8_t hexagon);
void releaseUpperHex	(uint8_t hexagon);

void touchTopLeftButton			(void);
void releaseTopLeftButton		(void);
void touchTopRightButton		(void);
void releaseTopRightButton		(void);
void touchBottomLeftButton		(void);
void releaseBottomLeftButton	(void);
void touchBottomRightButton		(void);
void releaseBottomRightButton	(void);


// OUTPUT
void dacSendPitchMode	(MantaSequencer seq, uint8_t step);
void dacSendTriggerMode	(MantaSequencer seq, uint8_t step);

// LEDs
void setPanelSelectLEDs		(void);
void setSliderLEDsFor		(MantaSequencer seq, int note);
void setKeyboardLEDsFor		(MantaSequencer seq, int note);
void setOptionLEDs			(void);
void setCompositionLEDs     (void);
void setSequencerLEDsFor	(MantaSequencer seq);
void setSequencerLEDs		(void);
void setTriggerPanelLEDsFor	(MantaSequencer seq, TriggerPanel panel);
void uiStep					(MantaSequencer seq);
void resetSliderMode		(void);

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
void downOctaveForEditStackSteps(MantaSequencer seq);
void upOctaveForEditStackSteps(MantaSequencer seq);

/* - - - - - - KEY PATTERNS - - - - - - - -*/

typedef enum TriggerPanelButtonType
{
	S1TrigPanelSelect = 0, 
	S2TrigPanelSelect,
	SXTrigPanelNil
	
} TriggerPanelButtonType;

typedef enum PitchPanelButtonType
{
	KeyboardPanelKeyC = 0,
	KeyboardPanelKeyCSharp = 1,
	KeyboardPanelKeyD = 2,
	KeyboardPanelKeyDSharp = 3,
	KeyboardPanelKeyE = 4,
	KeyboardPanelKeyF = 5,
	KeyboardPanelKeyFSharp = 6,
	KeyboardPanelKeyG = 7,
	KeyboardPanelKeyGSharp = 8,
	KeyboardPanelKeyA = 9, 
	KeyboardPanelKeyASharp = 10,
	KeyboardPanelKeyB = 11,
	KeyboardPanelRest = 255,
	KeyboardPanelOctaveDown = 253,
	KeyboardPanelOctaveUp = 254,
	KeyboardPanelGlide = 252,
	KeyboardEndOctave = 12
} PitchPanelButtonType;
// Upper keyboard pattern
uint8_t keyboard_pattern[16] = {KeyboardPanelKeyC,	    KeyboardPanelKeyD,		KeyboardPanelKeyE,	KeyboardPanelKeyF,		KeyboardPanelKeyG,		KeyboardPanelKeyA,		KeyboardPanelKeyB,		 KeyboardPanelRest, 
								KeyboardPanelKeyCSharp, KeyboardPanelKeyDSharp, KeyboardPanelGlide, KeyboardPanelKeyFSharp, KeyboardPanelKeyGSharp, KeyboardPanelKeyASharp, KeyboardPanelOctaveDown, KeyboardPanelOctaveUp};
	
uint8_t trigger_pattern[16] = 
{
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	S1TrigPanelSelect,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	SXTrigPanelNil,
	S2TrigPanelSelect,
	S2TrigPanelSelect,
	S2TrigPanelSelect,
	S2TrigPanelSelect
};

typedef enum GlobalOptionModeType
{
	LMonoKey = 0,
	LDuoKey,
	LPitchSeq,
	LTrigSeq,
	RMonoKey,
	RDuoKey,
	RPitchSeq,
	RTrigSeq,
	LSwitch,
	RSwitch,
	FullSplitQuad,
	GlobalOptionNil
} GlobalOptionModeType;

GlobalOptionModeType globalOptionButtons[16] =
{
	LMonoKey,
	LDuoKey,
	LPitchSeq,
	LTrigSeq,
	RMonoKey,
	RDuoKey,
	RPitchSeq,
	RTrigSeq,
	
	LSwitch,
	RSwitch,
	GlobalOptionNil,
	FullSplitQuad,
	GlobalOptionNil,
	GlobalOptionNil,
	GlobalOptionNil,
	GlobalOptionNil
};
	
	
// Additional options pattern

typedef enum OptionPanelButtonType
{
	OptionPatternOne = 0,
	OptionPatternTwo,
	OptionPatternThree,
	OptionPatternFour,
	OptionPatternFive,
	OptionPatternSix,
	OptionPatternSeven,
	OptionPatternEight,
	OptionPitch,
	OptionTrigger,
	OptionFullSplit,
	OptionLeft,
	OptionRight,
	OptionNil
	
}OptionPanelButtonType;

OptionPanelButtonType optionModeButtons[16] =	{	
	OptionPatternOne, 
	OptionPatternTwo, 
	OptionPatternThree, 
	OptionPatternFour, 
	OptionPatternFive,  
	OptionPatternSix, 
	OptionPatternSeven, 
	OptionPatternEight,
	
	OptionPitch, 
	OptionTrigger, 
	OptionNil,
	OptionFullSplit,  
	OptionNil, 
	OptionNil,
	OptionLeft, 
	OptionRight
};


uint8_t amberHexes[NUM_SEQ][MAX_STEPS];

int editNoteOn; 
int keyNoteOn;
int glideNoteOn;
int trigSelectOn;




tNoteStack editStack;
//tNoteStack noteOnFrame;
//tNoteStack noteOffFrame; // frames refreshed every 2 ms
tNoteStack noteOnStack; // all notes on at any point during runtime

uint8_t range_top = 15;
uint8_t range_bottom = 0;


uint16_t encodeBuffer[NUM_SEQ][sizeOfSerializedSequence]; 
uint16_t decodeBuffer[NUM_SEQ][sizeOfSerializedSequence];
uint16_t memoryInternalCompositionBuffer[NUM_SEQ][sizeOfBankOfSequences]; //8680 is 620 (number of bytes per sequence) * 14 (number of sequences that can be stored for each sequencer channel)

/* - - - - - - - - MantaState (touch events + history) - - - */
MantaSequencer currentSequencer = SequencerOne; // current Sequencer is CURRENTLY EDITING SEQUENCER

TriggerPanel currentPanel[2] = 
{
	PanelOne,
	PanelOne
};

// Flags for new inputs.
static uint8_t newUpperHexSeq = 0;
static uint8_t newLowerHexSeq = 0;
static uint8_t newReleaseLowerHexSeq = 0;
static uint8_t newReleaseUpperHexSeq = 0;
static uint8_t newFunctionButtonSeq = 0;
static uint8_t newReleaseFunctionButtonSeq = 0;

uint8_t prev_pattern_hex = 0;
uint8_t current_pattern_hex = 0;

uint8_t prev_panel_hex = 0;
uint8_t current_panel_hex = 0;

uint8_t prev_option_hex = 0;
uint8_t current_option_hex = 0;

uint8_t currentHexUIOff = 0;

uint8_t currentHexUI = 0;
uint8_t prevHexUI = 0;

uint8_t currentUpperHexUI = 0, prevUpperHexUI = 0;

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

bool blinkToggle;
void blink(void)
{
	blinkToggle = (blinkToggle == true) ? false : true;		   
	
	if (key_vs_option == OptionMode && compositionAction == CompositionCopy)
	{
		for (int seq = 0; seq < 2; seq++)
		{
			for (int comp = 0; comp < NUM_COMP; comp++)
			{
				if (compositionMap[seq][comp])
				{
					manta_set_LED_hex(seq * 16 + comp, blinkToggle ? AmberOn : AmberOff);
				}
			}
		}
		
		if (copyWhichSeq != SequencerNil && copyWhichComp != -1) 
			manta_set_LED_hex(copyWhichSeq * 16 + copyWhichComp, blinkToggle ? Red : Off);
	}
}

void initSequencer(void)
{
	sequencer_mode = 1;
	
	Write7Seg(preset_num); // shouldn't have to do this here, but there's a bug that writes garbage to the 7Seg when plugging in a Manta, and this seems to fix it
	initTimers();
	
	// Sequencer Modes
	pattern_type =				LeftRightRowUp;
	currentMantaSliderMode =	SliderModeOne;
	prevMantaSliderMode =		SliderModeOne;
	edit_vs_play =				PlayToggleMode; manta_set_LED_button(ButtonTopRight, Amber);
	currentFunctionButton =		ButtonTopLeft;
	key_vs_option =				KeyboardMode;
	playSubMode =				SeqMode; manta_set_LED_button(ButtonBottomRight, Off);
	
	tNoteStack_init(&editStack,		32);
	
	resetEditStack();
	
	// Initialize the noteOnStack. :D !!!
	tNoteStack_init(&noteOnStack, 32); 
	// Glad I commented that - thanks @Josh Becker for the tip on appropriate commenting.
	
	setCurrentSequencer(SequencerOne);
	keyNoteOn = -1;
	glideNoteOn = -1;
	
	//if preset_num == 0, then we are trying to initialize the blank default sequencer
	if (preset_num == 0)
	{
		seq1PvT = PitchMode; seq2PvT = PitchMode;
		for (int i = 0; i < NUM_SEQ; i++)
		{
			tSequencer_init(&sequencer[i], PitchMode, 32);
			
			tSequencer_encode(&sequencer[i], encodeBuffer);
			memoryInternalWriteSequencer(i, 0, encodeBuffer);
			
			compositionMap[i][0] = true;
			currentComp[i] = 0;
		}
	}

	//otherwise we are loading a saved preset, so we need to prepare that data properly
	else
	{
		seq1PvT = PitchMode; seq2PvT = PitchMode;
		for (int i = 0; i < NUM_SEQ; i++)
		{
			tSequencer_init(&sequencer[i], PitchMode, 32);
			tSequencer_encode(&sequencer[i], encodeBuffer);
			memoryInternalWriteSequencer(i, 0, encodeBuffer);
			compositionMap[i][0] = true;
			currentComp[i] = 0;
			tSequencer_decode(&sequencer[i], decodeBuffer);
		}
	}
	
	setSequencerLEDsFor(currentSequencer);
	
	setKeyboardLEDsFor(currentSequencer, -1);

	tc_start(tc3, TC3_CHANNEL);
	manta_send_LED();

}

void sequencerStep(void)
{
	
	int offset,cstep,curr;
	
	for (int seq = 0; seq < NUM_SEQ; seq++)
	{
		offset = seq * 2;
		
		cstep = sequencer[seq].currentStep;
		
		sequencer[seq].lengthCounter += 1;
		
		if (sequencer[seq].lengthCounter >= sequencer[seq].step[cstep].length)
		{
			tSequencer_next(&sequencer[seq]); // Move to next step, seq 1.
			
			curr = sequencer[seq].currentStep;
			
			if (sequencer[seq].stepGo)
			{
				if (sequencer[seq].pitchOrTrigger == PitchMode)
				{
					dacSendPitchMode(seq, curr);
				}
				else // TriggerMode
				{
					dacSendTriggerMode(seq, curr);
				}

				// Start sequencer 1 and 2 timers: tc1, tc2.
				// Timer callbacks tc1_irq and tc2_irq (in main.c) set dac trigger outputs low on every step.
				tc_start(tc1, TC1_CHANNEL);
			}
			sequencer[seq].lengthCounter = 0;
		}
	}
	
	// UI
	uiStep(currentSequencer);
	if (full_vs_split == SplitMode)
	{
		uiStep((currentSequencer+1) % NUM_SEQ);
	}
}

MantaButton lastFunctionButton;

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
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newHexUIOff = i;
			newReleaseLowerHexSeq = 1;
			
			tNoteStack_remove(&noteOnStack, i);
			
			releaseLowerHex(newHexUIOff);
			
			
		}
		
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			newHexUIOn = i;
			newLowerHexSeq = 1;
			
			tNoteStack_add(&noteOnStack, i);
			
			touchLowerHex(newHexUIOn);
			
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
			newUpperHexSeq = 1;
		}
		
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newUpperHexUI = i;
			newReleaseUpperHexSeq = 1;
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
			newFunctionButtonSeq = 1;
		}
		
		if ((func_button_states[i] <= 0) && (past_func_button_states[i] > 0))
		{
			currentFunctionButton = i;
			newReleaseFunctionButtonSeq = 1;
		}
		
		past_func_button_states[i] = func_button_states[i];
	}
	
	//if (newReleaseLowerHexSeq)		releaseLowerHex(newHexUIOff);
	
	//if (newLowerHexSeq)				touchLowerHex(newHexUIOn);
	
	if (newReleaseUpperHexSeq)		releaseUpperHex(newUpperHexUI);

	if (newUpperHexSeq)				touchUpperHex(newUpperHexUI);
	
	if (newFunctionButtonSeq)			
	{
		resetSliderMode();
		
		if (currentFunctionButton == ButtonTopLeft)				touchTopLeftButton();
		else if (currentFunctionButton == ButtonTopRight)		touchTopRightButton();
		else if (currentFunctionButton == ButtonBottomLeft)		touchBottomLeftButton();
		else if (currentFunctionButton == ButtonBottomRight)	touchBottomRightButton();
		
		newFunctionButtonSeq = 0;
	}
	
	if (newReleaseFunctionButtonSeq)
	{
		if (currentFunctionButton == ButtonTopLeft)				releaseTopLeftButton();
		else if (currentFunctionButton == ButtonTopRight)		releaseTopRightButton();
		else if (currentFunctionButton == ButtonBottomLeft)		releaseBottomLeftButton();
		else if (currentFunctionButton == ButtonBottomRight)	releaseBottomRightButton();
		
		newReleaseFunctionButtonSeq = 0;
	}

}

void releaseLowerHex(uint8_t hexagon)
{
	if (key_vs_option == OptionMode)
	{
		setCompositionLEDs();
	}
	
	if (edit_vs_play == EditMode)
	{

		if (noteOnStack.size > 0)		editNoteOn = editStack.notestack[0];
		else							editNoteOn = -1;
			
	
	}
	else if (edit_vs_play == PlayToggleMode)
	{
		int step = hexUIToStep(hexagon);
			
		if (playSubMode == ArpMode)
		{
			tSequencer_toggleStep(&sequencer[currentSequencer],step);
				
			manta_set_LED_hex(hexagon, Off);
		} 
		else // SeqMode
		{
			
		}
		
	}
	newReleaseLowerHexSeq = 0;
}

void clearSequencer(MantaSequencer seq)
{
	tSequencer_clearSteps(&sequencer[seq]);
	setSequencerLEDsFor(seq);
}

int first, last;

void setCompositionLEDs(void)
{
	for (int seq = 0 ; seq < 2; seq++)
	{
		for (int comp = 0; comp < NUM_COMP; comp++)
		{
			if (compositionMap[seq][comp])
			{
				if (compositionAction == CompositionRead)		manta_set_LED_hex(seq * 16 + comp, (currentComp[seq] == comp) ? Red : Amber);
				else if (compositionAction == CompositionWrite)	manta_set_LED_hex(seq * 16 + comp, Red);
			}
			
		}
		
		
		
	}
	
	manta_set_LED_hex(14, Amber);
	manta_set_LED_hex(15, Red);
	
	manta_set_LED_hex(30, Amber);
	manta_set_LED_hex(31, Red);
}




MantaSequencer whichSeq_test;
int whichComp_test;
void touchLowerHex(uint8_t hexagon)
{
	if (key_vs_option == OptionMode)
	{
		MantaSequencer whichSeq = (hexagon < 16) ? SequencerOne : SequencerTwo;
		int whichComp = (hexagon < 16) ? hexagon : (hexagon - 16);
		
		if (whichComp == 14)
		{
		
			manta_set_LED_hex(whichSeq * 16 + 14, Red);	
		}
		else if (whichComp == 15)
		{
			tSequencer_init(&sequencer[whichSeq], sequencer[whichSeq].pitchOrTrigger, 32);
			
			manta_set_LED_hex(whichSeq * 16 + 15, Amber);
		}
		else 
		{
			if (compositionAction == CompositionRead)
			{
				if (compositionMap[whichSeq][whichComp])
				{
					memoryInternalReadSequencer(whichSeq, whichComp, decodeBuffer);
					tSequencer_decode(&sequencer[whichSeq], decodeBuffer);
					
					currentComp[whichSeq] = whichComp;
					
					setCurrentSequencer(whichSeq);
					
					setOptionLEDs();
				}

			}
			else if (compositionAction == CompositionWrite) // CompositionWrite
			{
				manta_set_LED_hex(hexagon, Amber);
				
				compositionMap[whichSeq][whichComp] = true;
				tSequencer_encode(&sequencer[whichSeq], encodeBuffer);
				memoryInternalWriteSequencer(whichSeq, whichComp, encodeBuffer);
			}
			else if (compositionAction == CompositionCopy)
			{
				if (copyStage == 0)
				{
					// this determines sequence to be copied
					if (compositionMap[whichSeq][whichComp])
					{
						copyWhichSeq = whichSeq;
						copyWhichComp = whichComp;
						
						copyStage = 1;
					}
				}
				else if (copyStage == 1)
				{
					// any hex pressed while still in copy mode is written with sequencer from copystage 1
					if (!(whichSeq == copyWhichSeq && whichComp == copyWhichComp)) // if not the same step, copy
					{
						manta_set_LED_hex(hexagon, Red);
						compositionMap[whichSeq][whichComp] = true;
						memoryInternalCopySequencer(copyWhichSeq, copyWhichComp, whichSeq, whichComp);
					}
				}
				
			}
			
		}
		
		return;
		
	}

	if ((full_vs_split == SplitMode) && (edit_vs_play == TrigToggleMode) && ((hexagon < 16 && trigSelectOn >= 12) || (hexagon >= 16 && trigSelectOn < 4)))
	{
		//tNoteStack_clear(&noteOnFrame);
		newLowerHexSeq = 0;
		return;
	}
		
	if (tNoteStack_contains(&editStack,hexagon) < 0)	resetSliderMode();

	// Set hexUIs for this processing frame.
	prevHexUI = currentHexUI;
	currentHexUI = hexagon;

	MantaSequencer sequencerToSet = currentSequencer;
	if (full_vs_split == SplitMode)
	{
		if (hexagon < 16)	setCurrentSequencer(SequencerOne);
		else				setCurrentSequencer(SequencerTwo);
			
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
		
	int step = hexUIToStep(hexagon);
			
	if (edit_vs_play == EditMode)
	{
		if (editNoteOn >= 0)
		{
			// If hex is not in editStack, add it
			if (tNoteStack_contains(&editStack, hexagon) < 0)
			{
				tNoteStack_add(&editStack, hexagon);
				manta_set_LED_hex(hexagon, Red);
			}
			else
			{
				tNoteStack_remove(&editStack, hexagon);

				if (amberHexes[currentSequencer][hexagon] == 1)	manta_set_LED_hex(hexagon, Amber);
				else											manta_set_LED_hex(hexagon, Off);
			}
		}
		else
		{
			resetEditStack();
			setSequencerLEDsFor(sequencerToSet);
					
			editNoteOn = hexagon;
			manta_set_LED_hex(editNoteOn, Red);
		}

	}
	else if (edit_vs_play == PlayToggleMode)
	{
		tNoteStack_remove(&editStack, prevHexUI);
		tNoteStack_add(&editStack, currentHexUI);
				
		if (playSubMode == SeqMode) // note ons should toggle sequencer steps in and out of the pattern
		{
			if (tSequencer_toggleStep(&sequencer[currentSequencer], step))
			{
				manta_set_LED_hex(hexagon, AmberOn);
				amberHexes[currentSequencer][hexagon] = 1;
			}
			else
			{
				manta_set_LED_hex(hexagon, AmberOff);
				amberHexes[currentSequencer][hexagon] = 0;
				if (hexagon == uiHexCurrentStep)
				{
					manta_set_LED_hex(hexagon, RedOff);
				}
			}
		}
		else // ArpMode
		{
			tSequencer_toggleStep(&sequencer[currentSequencer], step);
			manta_set_LED_hex(hexagon, AmberOn);
		}
	}
	else // TrigToggleMode
	{
		tNoteStack_remove(&editStack, prevHexUI);
		tNoteStack_add(&editStack,currentHexUI);
				
		int currPanel = currentPanel[currentSequencer];
				
		int cont = 1;
				
		if (cont)
		{
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
		setOptionLEDs();
	}
		
	if (editStack.size > 1)
	{
		setSliderLEDsFor(currentSequencer, -1);
	}
	else
	{
		setSliderLEDsFor(currentSequencer, hexUIToStep(hexagon));
	}
	
	
	newLowerHexSeq = 0;
}

void switchToMode(MantaEditPlayMode mode)
{
	edit_vs_play = mode;
	
	if (mode == TrigToggleMode)
	{
		if (full_vs_split == SplitMode)
		{
			if (trigSelectOn >= 0 && trigSelectOn < 4)	setTriggerPanelLEDsFor(SequencerOne,currentPanel[SequencerOne]);
			else										setTriggerPanelLEDsFor(SequencerTwo,currentPanel[SequencerTwo]);
			
			manta_set_LED_hex(stepToHexUI(SequencerOne, sequencer[SequencerOne].currentStep), Amber);
			manta_set_LED_hex(stepToHexUI(SequencerTwo, sequencer[SequencerTwo].currentStep), Amber);
		}
		else // FullMode
		{
			setTriggerPanelLEDsFor(currentSequencer,currentPanel[currentSequencer]);
			
			manta_set_LED_hex(stepToHexUI(currentSequencer, sequencer[currentSequencer].currentStep), Amber);
			
		}
		
	}
	else if (mode == PlayToggleMode)
	{
		if (full_vs_split == SplitMode)
		{
			setSequencerLEDsFor(SequencerOne);
			setSequencerLEDsFor(SequencerTwo);
		}
		else // FullMode
		{
			setSequencerLEDsFor(currentSequencer);
		}
		manta_set_LED_button(ButtonTopRight, Amber);
	}
	else // EditMode
	{
		int currHex = tNoteStack_first(&editStack);
		
		if (currHex != sequencer[currentSequencer].currentStep)
		{
			manta_set_LED_hex(currHex, Red);
		}
		else
		{
			manta_set_LED_hex(currHex, Amber);
		}
	
		manta_set_LED_button(ButtonTopRight, Red);
	}
}


void releaseUpperHex(uint8_t hexagon)
{
	if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
	{
		if (keyboard_pattern[hexagon-MAX_STEPS] < KeyboardEndOctave)
		{
			// Exit SliderModePitch
			if (keyNoteOn == hexagon)
			{
				keyNoteOn = -1;
			
				currentMantaSliderMode = prevMantaSliderMode;
				if (editStack.size <= 1)
				{
					setSliderLEDsFor(currentSequencer, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentSequencer, -1);
				}
			}
			
		}
		else if (keyboard_pattern[hexagon-MAX_STEPS] == KeyboardPanelGlide)
		{
			if (glideNoteOn == hexagon)
			{
				glideNoteOn = -1;
				
				currentMantaSliderMode = prevMantaSliderMode;
				if (editStack.size <= 1)
				{
					setSliderLEDsFor(currentSequencer, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentSequencer, -1);
				}
				
				manta_set_LED_hex(hexagon, Off);
			}
		}
		
		if (key_vs_option == KeyboardMode)
		{
			if ((keyboard_pattern[hexagon-MAX_STEPS] == KeyboardPanelOctaveDown) || (keyboard_pattern[hexagon-MAX_STEPS] == KeyboardPanelOctaveUp))
			{
				currentMantaSliderMode = prevMantaSliderModeForOctaveHexDisable;
				
				if (currentMantaSliderMode != SliderModeThree)
				{
					setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
				}
			}
		}
	}
	else // TriggerMode
	{
		int whichUpperHex = hexagon - MAX_STEPS;
		int whichTrigPanel = trigger_pattern[whichUpperHex];
		

		if (whichTrigPanel != S1TrigPanelSelect && whichTrigPanel != S2TrigPanelSelect)
		{
			if (glideNoteOn == hexagon)
			{
				glideNoteOn = -1;
				
				currentMantaSliderMode = prevMantaSliderMode;
				if (editStack.size <= 1)
				{
					setSliderLEDsFor(currentSequencer, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentSequencer, -1);
				}
				
				manta_set_LED_hex(hexagon, Off);
			}
		}
		else 
		{
			if ((trigSelectOn+MAX_STEPS) == hexagon)
			{
				 trigSelectOn = -1;
				 
				 manta_set_LED_hex(hexagon, Amber);
				 
				 switchToMode(PlayToggleMode);
			}
		}
		
	}
	
	newReleaseUpperHexSeq = 0;
}

void allUIStepsOff(MantaSequencer whichSeq)
{
	int hexUI = 0;
	for (int i = 0; i < sequencer[whichSeq].maxLength; i++)
	{
		hexUI = stepToHexUI(whichSeq,i);
		
		manta_set_LED_hex(hexUI ,Off);
	}
}

void touchUpperHex(uint8_t hexagon)
{
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;
	

	if (key_vs_option == KeyboardMode)
	{
		if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
		{
			int upperHexType = keyboard_pattern[hexagon-MAX_STEPS];
				
			if (upperHexType == KeyboardPanelRest)
			{
				// handle case when more than one note in editStack (check if they all have same parameter value
				// need to be able to set multiple notes to rest too
				int step = hexUIToStep(tNoteStack_first(&editStack));
					
				if (!(getParameterFromStep(currentSequencer, step, Note)))	setParameterForEditStackSteps(currentSequencer,Note,1);
				else														setParameterForEditStackSteps(currentSequencer,Note,0);
			}
			else if (upperHexType == KeyboardPanelOctaveDown)
			{
				// down an octave
				tSequencer_downOctave(&sequencer[currentSequencer]);
				downOctaveForEditStackSteps(currentSequencer);
				
				// Temporarily set slider LEDs to display current octave.
				if (editStack.size > 1) setSliderLEDsFor(currentSequencer, -1);
				else					manta_set_LED_slider(SliderOne, sequencer[currentSequencer].step[hexUIToStep(editStack.notestack[0])].octave + 1);
				
				
				if (currentMantaSliderMode != SliderModeNil)
				{
					prevMantaSliderModeForOctaveHexDisable = currentMantaSliderMode;
					currentMantaSliderMode = SliderModeNil;
				}
				
				// setParameterForEditStackSteps(currentSequencer,Octave,sequencer[currentSequencer].octave);
			}
			else if (upperHexType == KeyboardPanelOctaveUp)
			{
				//up an octave
				tSequencer_upOctave(&sequencer[currentSequencer]);
				upOctaveForEditStackSteps(currentSequencer);
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				
				// Temporarily set slider LEDs to display current octave.
				if (editStack.size > 1) setSliderLEDsFor(currentSequencer, -1);
				else					manta_set_LED_slider(SliderOne, sequencer[currentSequencer].step[hexUIToStep(editStack.notestack[0])].octave + 1);
				
				if (currentMantaSliderMode != SliderModeNil) 
				{
					prevMantaSliderModeForOctaveHexDisable = currentMantaSliderMode;
					currentMantaSliderMode = SliderModeNil;
				}
				// setParameterForEditStackSteps(currentSequencer,Octave,sequencer[currentSequencer].octave);
			}
			else if (upperHexType == KeyboardPanelGlide) 
			{
				if (glideNoteOn == -1)
				{
					glideNoteOn = hexagon;
					
					// Enter SliderModeGlide
					prevMantaSliderMode = currentMantaSliderMode;
					currentMantaSliderMode = SliderModeGlide;
					
					manta_set_LED_hex(hexagon, Red);
					
					if (editStack.size <= 1)
					{
						setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentSequencer, -1);
					}
				}
				
			}
			else //KeyboardPanelKeyX
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
						setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentSequencer, -1);
					}
				}
			}
				
			int cStep = sequencer[currentSequencer].currentStep;
			if (tNoteStack_contains(&editStack,stepToHexUI(currentSequencer,cStep)) != -1)
			{
				//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, cStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
				

			setKeyboardLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
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
				if (sequencer[SequencerOne].pitchOrTrigger == TriggerMode)
				{
					whichSeq = SequencerOne;
					whichPanel = whichUpperHex;
					
					if (edit_vs_play == PlayToggleMode)
					{
						trigSelectOn = whichUpperHex;
						switchToMode(TrigToggleMode);
					}
					
					cont= 1;
				}
			}
			else if (whichTrigPanel == S2TrigPanelSelect)
			{
				if (sequencer[SequencerTwo].pitchOrTrigger == TriggerMode)
				{
					whichSeq = SequencerTwo;
					whichPanel = whichUpperHex - 12;
					
					if (edit_vs_play == PlayToggleMode)
					{
						trigSelectOn = whichUpperHex;
						switchToMode(TrigToggleMode);
					}
					
					cont= 1;
				}
			}
			else
			{
				if (glideNoteOn == -1)
				{
					glideNoteOn = hexagon;
					
					// Enter SliderModeGlide
					prevMantaSliderMode = currentMantaSliderMode;
					currentMantaSliderMode = SliderModeGlide;
					
					manta_set_LED_hex(hexagon, Red);
					
					if (editStack.size <= 1)
					{
						setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentSequencer, -1);
					}
				}
			}
			
			if (cont) //dumb method
			{
				if (whichSeq != currentSequencer)
				{
					setCurrentSequencer(whichSeq);
				
					if (edit_vs_play != TrigToggleMode)
					{
						setKeyboardLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
						setSequencerLEDsFor(currentSequencer);
					}
				}
			
				int uiOffset = MAX_STEPS;
			
				if (whichSeq == SequencerTwo) uiOffset += 12;
			
				if (edit_vs_play == EditMode)
				{
					if (whichTrigPanel != SXTrigPanelNil)
					{
						int step = hexUIToStep(tNoteStack_first(&editStack));
					
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
				
					setTriggerPanelLEDsFor(currentSequencer, currentPanel[currentSequencer]);
					
					allUIStepsOff((currentSequencer+1)%2);
				
					manta_set_LED_hex(currentUpperHexUI, Red);
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
		
		OptionPanelButtonType whichOptionType = optionModeButtons[whichHex];
		
		if (whichOptionType <= OptionPatternEight)
		{
			tSequencer_setPattern(&sequencer[currentSequencer], whichOptionType);
				
			prev_pattern_hex = current_pattern_hex;
			current_pattern_hex = whichHex;
		}
		else if (whichOptionType == OptionPitch)
		{
			resetEditStack();
			
			prev_option_hex = current_option_hex;
			current_option_hex = whichHex;
			
			currentHexUI = 0;
				
			currentUpperHexUI = 0;

			if (sequencer[currentSequencer].pitchOrTrigger != PitchMode)
			{
				tSequencer_init(&sequencer[currentSequencer], PitchMode, MAX_STEPS); seq1PvT = PitchMode;
				seq1PvT = (currentSequencer == SequencerOne) ? PitchMode : seq1PvT;
				seq2PvT = (currentSequencer == SequencerTwo) ? PitchMode : seq2PvT;
			}
			
		}
		else if (whichOptionType == OptionTrigger)
		{
			resetEditStack();
			
			prev_option_hex = current_option_hex;
			current_option_hex = whichHex;
			
			currentHexUI = 0;
			
			currentUpperHexUI = 0;

			if (sequencer[currentSequencer].pitchOrTrigger != TriggerMode)
			{
				tSequencer_init(&sequencer[currentSequencer], TriggerMode, MAX_STEPS); 
				seq1PvT = (currentSequencer == SequencerOne) ? TriggerMode : seq1PvT;
				seq2PvT = (currentSequencer == SequencerTwo) ? TriggerMode : seq2PvT;
			}
		}
		else if (whichOptionType == OptionFullSplit)
		{
			full_vs_split = (full_vs_split == FullMode) ? SplitMode : FullMode;
		}
		else if (whichOptionType == OptionLeft)
		{
			prev_panel_hex = current_panel_hex;
			current_panel_hex = whichHex;
			
			setCurrentSequencer(SequencerOne);
		}
		else if (whichOptionType == OptionRight)
		{
			prev_panel_hex = current_panel_hex;
			current_panel_hex = whichHex;
			
			setCurrentSequencer(SequencerTwo);
		}
		
		setOptionLEDs();
	}

	//set memory variables
	newUpperHexSeq = 0;
}

void resetSliderMode(void)
{
	if (keyNoteOn >= 0 )
	{
		keyNoteOn = -1;
		currentMantaSliderMode = prevMantaSliderMode;
		setSliderLEDsFor(currentSequencer, hexUIToStep(current_pattern_hex));
	}
	
	if (glideNoteOn >= 0)
	{
		glideNoteOn = -1;
		currentMantaSliderMode = prevMantaSliderMode;
		setSliderLEDsFor(currentSequencer, hexUIToStep(current_pattern_hex));
	}
}

// ~ ~ ~ ~ TOP LEFT BUTTON ~ ~ ~ ~ //
void touchTopLeftButton(void)
{
	if (key_vs_option == OptionMode) return;
	
	//toggle three different SliderModes in PitchMode
	if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
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
			setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
		}
	}
	
}

void releaseTopLeftButton(void)
{
	if (key_vs_option == OptionMode) return;
	
}

// ~ ~ ~ ~ TOP RIGHT BUTTON ~ ~ ~ ~ //
void touchTopRightButton(void)
{
	if (key_vs_option == OptionMode)
	{
		compositionAction = CompositionCopy;
		
		return;
	}
	
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
	
	if (edit_vs_play == PlayToggleMode)
	{
		switchToMode(EditMode);
	}
	else // EditMode
	{
		switchToMode(PlayToggleMode);
	}
	
	setSliderLEDsFor(currentSequencer, hexUIToStep(tNoteStack_first(&editStack)));
}

void releaseTopRightButton(void)
{
	compositionAction = CompositionRead;
	copyStage = 0;
	copyWhichSeq = SequencerNil;
	copyWhichComp = -1;
	
	if (key_vs_option == OptionMode) setCompositionLEDs();
	else                             setSequencerLEDs();
}

// ~ ~ ~ ~ BOTTOM LEFT BUTTON ~ ~ ~ ~ //
void touchBottomLeftButton(void)
{
	key_vs_option = OptionMode;
	
	allUIStepsOff(SequencerOne);
	allUIStepsOff(SequencerTwo);
	
	setOptionLEDs();

	manta_set_LED_button(ButtonBottomLeft, Red);
}

void setSequencerLEDs(void)
{
	if (full_vs_split == FullMode)
	{
		setSequencerLEDsFor(currentSequencer);
	}
	else //SplitMode
	{
		setSequencerLEDsFor(SequencerOne);
		setSequencerLEDsFor(SequencerTwo);
	}
}

void releaseBottomLeftButton(void)
{	
	key_vs_option = KeyboardMode;
	
	if (edit_vs_play == TrigToggleMode)		setKeyboardLEDsFor(currentSequencer, 0);
	else /* PlayToggleMode or EditMode */	setKeyboardLEDsFor(currentSequencer, -1);
	
	setSequencerLEDs();

	manta_set_LED_button(ButtonBottomLeft, Off);
}

// ~ ~ ~ ~ BOTTOM RIGHT BUTTON ~ ~ ~ ~ //
void touchBottomRightButton(void)
{
	if (key_vs_option == OptionMode)
	{
		compositionAction = CompositionWrite;
		setCompositionLEDs();
	}
	else 
	{
		if (playSubMode == SeqMode)
		{
			playSubMode = ArpMode;
			manta_set_LED_button(ButtonBottomRight, Amber);
		}
		else // ArpMode
		{
			playSubMode = SeqMode;
			manta_set_LED_button(ButtonBottomRight, Off);
		}
	}
}

void releaseBottomRightButton(void)
{
	compositionAction = CompositionRead;
	
	if (key_vs_option == OptionMode)	setCompositionLEDs();
	else								setSequencerLEDs();
}

uint16_t testval = 0;
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
			setParameterForEditStackSteps(currentSequencer, CV2, val << 4);
		}
		
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off

		if (tNoteStack_contains(&editStack, uiHexCurrentStep) != -1)
		{
			if (sliderNum == SliderOne)
			{
				dacsend(currentSequencer * 2, 1, val);
			}
			else
			{
				testval = val << 5;
				DAC16Send(2*currentSequencer+1, val << 4);
			}
			
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
		
		if (tNoteStack_contains(&editStack,uiHexCurrentStep) != -1)
		{
			if (sliderNum == SliderOne)
			{
				dacsend(currentSequencer * 2 + 1, 0, val);
			}
			else
			{
				dacsend(currentSequencer * 2 + 1, 1, val);
			}
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
				prevOct = sequencer[currentSequencer].step[hexUIToStep(tNoteStack_first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentSequencer,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
			{
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, currStep)); 
			}
		}
		else //SliderTwo
		{
			setParameterForEditStackSteps(currentSequencer, Length, (val >> 9) + 1);
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
				prevOct = sequencer[currentSequencer].step[hexUIToStep(tNoteStack_first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentSequencer,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
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
				prevFine = sequencer[currentSequencer].step[hexUIToStep(tNoteStack_first(&editStack))].fine;
			}
			uint16_t newFine = val;
			
			setParameterForEditStackSteps(currentSequencer, Fine, val);
			
			manta_set_LED_slider(SliderTwo, (val >> 9) + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevFine != newFine))
			{
				DAC16Send(2 * currentSequencer, get16BitPitch(currentSequencer, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
			
		}
	}
	else if (currentMantaSliderMode == SliderModeGlide)
	{
		if (sliderNum == SliderOne)
		{
			// Pitch glide time
			uint16_t prevGlide = 0;
			if (editStack.size <= 1)
			{
				if (sequencer[currentSequencer].pitchOrTrigger == PitchMode) 
					prevGlide = sequencer[currentSequencer].step[hexUIToStep(tNoteStack_first(&editStack))].pglide;
				else
					prevGlide = sequencer[SequencerTwo].step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
				
			}
			uint16_t newGlide = val;
			
			if (newGlide < 5) newGlide = 5;
			
			if (sequencer[currentSequencer].pitchOrTrigger == PitchMode) 
				setParameterForEditStackSteps(currentSequencer, PitchGlide, newGlide);
			else
				setParameterForEditStackSteps(SequencerTwo, CVGlide, newGlide);
				
			manta_set_LED_slider(SliderOne, (val >> 9)  + 1); // add one to the slider values because a zero turns them off
		}
		else // SliderTwo
		{
			// CV glide time
			uint16_t prevGlide = 0;
			if (editStack.size <= 1)
			{
				if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
					prevGlide = sequencer[currentSequencer].step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
				else
					prevGlide = sequencer[SequencerOne].step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
			}
			uint16_t newGlide = val;
			
			if (newGlide < 5) newGlide = 5;
			
			if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
				setParameterForEditStackSteps(currentSequencer, CVGlide, newGlide);
			else
				setParameterForEditStackSteps(SequencerOne, CVGlide, newGlide);
			
			manta_set_LED_slider(SliderTwo, (val >> 9)  + 1); // add one to the slider values because a zero turns them off
		}
	}
	else
	{
		// Should not get here.
	}
}

void uiStep(MantaSequencer seq)
{
	if (key_vs_option == OptionMode) return;
	
	int cStep = sequencer[seq].currentStep;
	int pStep = sequencer[seq].prevStep;
	int uiHexCurrentStep = stepToHexUI(seq, cStep);
	int uiHexPrevStep = stepToHexUI(seq, pStep);

	if (edit_vs_play == EditMode)
	{
		if (sequencer[seq].pitchOrTrigger == PitchMode)
		{
			if (tNoteStack_contains(&editStack, uiHexCurrentStep) >= 0)
			{
				manta_set_LED_hex(uiHexCurrentStep, AmberOn);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, RedOn);
			}
			
			if (tNoteStack_contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, AmberOff);
			}
			else
			{
				manta_set_LED_hex(uiHexPrevStep, RedOff);
			}
		}
		else // TriggerMode
		{
			if (tNoteStack_contains(&editStack,uiHexCurrentStep) >= 0)
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
			
			if (tNoteStack_contains(&editStack,uiHexPrevStep) >= 0)
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
		if (sequencer[seq].pitchOrTrigger == TriggerMode)
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
			if (sequencer[seq].notestack.size > 0) manta_set_LED_hex(uiHexCurrentStep, RedOn);
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
	manta_set_LED_hex(currentPanel[SequencerOne] + MAX_STEPS, Red);
	manta_set_LED_hex(currentPanel[SequencerTwo] + MAX_STEPS + 12, Red);	
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
	
	if (sequencer[seq].pitchOrTrigger == PitchMode)
	{
		if (sequencer[seq].step[nt].note == 1)
		{
			for (int j = 0; j < 16; j++)
			{
				if (keyboard_pattern[j] < KeyboardEndOctave)
				{
					manta_set_LED_hex(j+MAX_STEPS, Amber);
				}
				else if (keyboard_pattern[j] == KeyboardPanelRest)
				{
					manta_set_LED_hex(j+MAX_STEPS,Red);
				}
				else if (keyboard_pattern[j] == KeyboardPanelGlide)
				{
					manta_set_LED_hex(j+MAX_STEPS, Off);
				}
				else
				{
					manta_set_LED_hex(j+MAX_STEPS, Off);
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
				if (keyboard_pattern[j] < KeyboardEndOctave)
				{
					manta_set_LED_hex(j+MAX_STEPS, Red);
				}
				else if (keyboard_pattern[j] == KeyboardPanelRest)
				{
					manta_set_LED_hex(j+MAX_STEPS,Amber);
				}
				else if (keyboard_pattern[j] == KeyboardPanelGlide)
				{
					
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
		
		if (seq == SequencerTwo)
		{
			hexUIOffset += 12;
		}
		
		if (setRed)
		{
			for (int panel = 0; panel < 4; panel++)
			{
				manta_set_LED_hex(hexUIOffset + panel, (sequencer[seq].step[note].on[panel]) ? Red : Amber);
			}
		}		
	}
	
	manta_set_LED_button(ButtonTopRight, (edit_vs_play == EditMode) ? Red : Amber);
	manta_set_LED_button(ButtonBottomLeft, Off);
	manta_set_LED_button(ButtonBottomRight, (playSubMode == SeqMode) ? Off : Amber);
	manta_set_LED_button(ButtonTopLeft, (currentMantaSliderMode == SliderModeOne) ? Off : (currentMantaSliderMode == SliderModeTwo) ? Amber : Red);
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
	uint16_t pglide = 0;
	uint16_t cvglide = 0;
	
	if (note >= 0)
	{
		cv1 = (sequencer[seq].step[note].cv1 >> 9) + 1;
		cv2 = (sequencer[seq].step[note].cv2 >> 9) + 1;
		cv3 = (sequencer[seq].step[note].cv3 >> 9) + 1;
		cv4 = (sequencer[seq].step[note].cv4 >> 9) + 1;
		fine = (sequencer[seq].step[note].fine >> 9) + 1;
		octave = (sequencer[seq].step[note].octave + 1);
		length = (sequencer[seq].step[note].length);
		pglide = (sequencer[seq].step[note].pglide >> 9) + 1;
		cvglide = (sequencer[seq].step[note].cvglide >> 9) + 1;
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
	else if (currentMantaSliderMode == SliderModeGlide)
	{
		if (sequencer[seq].pitchOrTrigger == PitchMode)
		{
			manta_set_LED_slider(SliderOne, pglide); // OCTAVE add one to the slider values because a zero turns them off
			manta_set_LED_slider(SliderTwo, cvglide); // the step length is already between 1-8
		}
		else // TriggerMode
		{
			manta_set_LED_slider(SliderOne, (sequencer[SequencerOne].step[note].cvglide >> 9) + 1); 
			manta_set_LED_slider(SliderTwo, (sequencer[SequencerTwo].step[note].cvglide >> 9) + 1); 
		}
		
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
		
		manta_set_LED_hex(hexUI,Off);
		
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

		manta_set_LED_hex(hexUI, (sequencer[seq].step[i].toggled) ? Amber : Off);
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

void setOptionLEDs(void)
{
// ALSO SET COMPOSITION MODE LEDS
	//change the keyboard LEDs to be the MODE leds
	OptionPanelButtonType option = OptionNil;
	uint8_t hex = 64;
	for (int i = 0; i < 16; i++)
	{
		option = optionModeButtons[i];
		hex = i + MAX_STEPS;
		
		if (option == OptionNil)
		{
			manta_set_LED_hex(hex, Off);
		}
		else if (option == OptionPitch)
		{
			manta_set_LED_hex(hex, (sequencer[currentSequencer].pitchOrTrigger == PitchMode) ? Red : Amber);
		}
		else if (option == OptionTrigger)
		{
			manta_set_LED_hex(hex, (sequencer[currentSequencer].pitchOrTrigger == TriggerMode) ? Red : Amber);
		}
		else if (option == OptionFullSplit)
		{
			manta_set_LED_hex(hex, (full_vs_split == FullMode) ? Amber : Red);
		}
		else if (option <= OptionPatternEight)
		{
			manta_set_LED_hex(hex, (option == sequencer[currentSequencer].pattern) ? Red : Amber);
		}
		else if (option == OptionLeft)
		{
			manta_set_LED_hex(hex, (currentSequencer == SequencerOne) ? Red : Amber);
		}
		else if (option == OptionRight)
		{
			manta_set_LED_hex(hex, (currentSequencer == SequencerTwo) ? Red : Amber);
		}
	}

	manta_set_LED_button(ButtonBottomRight, Red); // Write
	manta_set_LED_button(ButtonTopRight, Amber); // Copy
	manta_set_LED_button(ButtonTopLeft, Off); // Nothing for now
	
	setCompositionLEDs();

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

uint16_t glideTimeTEMP = 0;
float destTEMP= .0f;

void dacSendPitchMode(MantaSequencer seq, uint8_t step)
{
	if (sequencer[seq].step[step].note)
	{
		// Configure PitchGlide
		tRamp* glide = &out[seq][CVPITCH];
		
		uint16_t glideTime =  sequencer[seq].step[step].pglide >> 3;
		if (glideTime < 5) glideTime = 5;
		
		tRampSetDest(glide, (float)get16BitPitch(seq,step) / UINT16_MAX); 
		tRampSetTime(glide, glideTime);

		// Configure CVGlide
		glideTime =  sequencer[seq].step[step].cvglide >> 3;
		if (glideTime < 5) glideTime = 5;
		

		tRampSetTime(&out[seq][CV1P], glideTime);
		tRampSetDest(&out[seq][CV1P], (float) sequencer[seq].step[step].cv1);
		
		tRampSetTime(&out[seq][CV2P], glideTime);
		tRampSetDest(&out[seq][CV2P], (float) sequencer[seq].step[step].cv2);
		
		tRampSetTime(&out[seq][CV3P], glideTime);
		tRampSetDest(&out[seq][CV3P], (float) sequencer[seq].step[step].cv3);
		
		tRampSetTime(&out[seq][CV4P], glideTime);
		tRampSetDest(&out[seq][CV4P], (float) sequencer[seq].step[step].cv4);
		
		// Send Trigger
		dacsend(2*seq,0,4095);
	}
}


void dacSendTriggerMode(MantaSequencer seq, uint8_t step)
{
	// Configure CVGlide
	uint16_t glideTime =  sequencer[seq].step[step].cvglide >> 3;
	if (glideTime < 5) glideTime = 5;
	

	tRampSetTime(&out[seq][CV1T], glideTime);
	tRampSetDest(&out[seq][CV1T], (float) sequencer[seq].step[step].cv1);
		
	tRampSetTime(&out[seq][CV2T], glideTime);
	tRampSetDest(&out[seq][CV2T], (float) sequencer[seq].step[step].cv2);

	
	// Trigger 1, Trigger 2, Trigger 3, Trigger 4
	dacsend(2*seq+0, 0, sequencer[seq].step[step].on[0] * 4095);
	dacsend(2*seq+1, 0, sequencer[seq].step[step].on[1] * 4095);
	dacsend(2*seq+0, 1, sequencer[seq].step[step].on[2] * 4095);
	dacsend(2*seq+1, 1, sequencer[seq].step[step].on[3] * 4095);
}

// UTILITIES
uint16_t getParameterFromStep(MantaSequencer seq, uint8_t step, StepParameterType param)
{
	if (param == Toggled)		return sequencer[seq].step[step].toggled;
	else if (param == Length)	return sequencer[seq].step[step].length;
	else if (param == CV1)		return sequencer[seq].step[step].cv1;
	else if (param == CV2)		return sequencer[seq].step[step].cv2;
	else if (param == CV3)		return sequencer[seq].step[step].cv3;
	else if (param == CV4)		return sequencer[seq].step[step].cv4;
	else if (param == Pitch)	return sequencer[seq].step[step].pitch;
	else if (param == Fine)		return sequencer[seq].step[step].fine;
	else if (param == Octave)	return sequencer[seq].step[step].octave;
	else if (param == Note)		return sequencer[seq].step[step].note;
	else if (param == KbdHex)	return sequencer[seq].step[step].kbdhex;
	else if (param == PitchGlide)	return sequencer[seq].step[step].pglide;
	else if (param == CVGlide)	return sequencer[seq].step[step].cvglide;
	else if (param == On1)		return sequencer[seq].step[step].on[PanelOne];
	else if (param == On2)		return sequencer[seq].step[step].on[PanelTwo];
	else if (param == On3)		return sequencer[seq].step[step].on[PanelThree];
	else if (param == On4)		return sequencer[seq].step[step].on[PanelFour];
	else return 0;
}

void setParameterForStep(MantaSequencer seq, uint8_t step, StepParameterType param, uint16_t value)
{
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)			sequencer[seq].step[step].toggled = value;
	else if (param == Length)		sequencer[seq].step[step].length = value;
	else if (param == CV1)			sequencer[seq].step[step].cv1 = value;
	else if (param == CV2)			sequencer[seq].step[step].cv2 = value;
	else if (param == CV3)			sequencer[seq].step[step].cv3 = value;
	else if (param == CV4)			sequencer[seq].step[step].cv4 = value;
	else if (param == Pitch)		sequencer[seq].step[step].pitch = value;
	else if (param == Fine)			sequencer[seq].step[step].fine = value;
	else if (param == Octave)		sequencer[seq].step[step].octave = value;
	else if (param == Note)			sequencer[seq].step[step].note = value;
	else if (param == KbdHex)		sequencer[seq].step[step].kbdhex = value;
	else if (param == PitchGlide)		sequencer[seq].step[step].pglide = value;
	else if (param == CVGlide)		sequencer[seq].step[step].cvglide = value;
	else if (param == On1)			sequencer[seq].step[step].on[PanelOne] = value;
	else if (param == On2)			sequencer[seq].step[step].on[PanelTwo] = value;
	else if (param == On3)			sequencer[seq].step[step].on[PanelThree] = value;
	else if (param == On4)			sequencer[seq].step[step].on[PanelFour] = value;
	else;
}

void setParameterForEditStackSteps(MantaSequencer seq, StepParameterType param, uint16_t value)
{
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].toggled = value;
	else if (param == Length)		for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].length = value;
	else if (param == CV1)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv1 = value;
	else if (param == CV2)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv2 = value;
	else if (param == CV3)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv3 = value;
	else if (param == CV4)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cv4 = value;
	else if (param == Pitch)		for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].pitch = value;
	else if (param == Fine)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].fine = value;
	else if (param == Octave)		for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = value;
	else if (param == Note)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].note = value;
	else if (param == KbdHex)		for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].kbdhex = value;
	else if (param == PitchGlide)   for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].pglide = value;
	else if (param == CVGlide)		for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].cvglide = value;
	else if (param == On1)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelOne] = value;
	else if (param == On2)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelTwo] = value;
	else if (param == On3)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelThree] = value;
	else if (param == On4)			for (; i < size; i++) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].on[PanelFour] = value;
	else;
}

void upOctaveForEditStackSteps(MantaSequencer seq) {
	for (int i = 0; i < editStack.size; i++) {
		int value = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave;
		if (value >= 7) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave;
		else			sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave + 1;
	}
}

void downOctaveForEditStackSteps(MantaSequencer seq) {
	for (int i = 0; i < editStack.size; i++) {
		int value = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave;
		if (value <= 0) sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave;
		else			sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave = sequencer[seq].step[hexUIToStep(editStack.notestack[i])].octave - 1;
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

// Resets edit stack so it only contains most recently touched hex.
// Sets shift/noteon hex to -1 to prevent notes from being added to edit stack until new note is pressed.
void resetEditStack(void)
{
	tNoteStack_clear(&editStack);
	editNoteOn = -1;
	if (currentHexUI >= 0 && currentHexUI < MAX_STEPS)	tNoteStack_add(&editStack,currentHexUI);
	else												tNoteStack_add(&editStack,0);
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


//maybe would be more efficient with memcpy? Not sure if that would be better than an iterated array in this case -JS
void memoryInternalReadSequencer(int whichSeq, int whichhex, uint16_t* buffer)
{
	for (int i = 0; i < sizeOfSerializedSequence; i++)
	{
		buffer[i] = memoryInternalCompositionBuffer[whichSeq][(whichhex*sizeOfSerializedSequence) + i];
	}
}

void memoryInternalWriteSequencer(int whichSeq, int whichhex, uint16_t* buffer)
{
	for (int i = 0; i < sizeOfSerializedSequence; i++)
	{
		memoryInternalCompositionBuffer[whichSeq][(whichhex*sizeOfSerializedSequence) + i] = buffer[i];
	}
}

void memoryInternalCopySequencer(int sourceSeq, int sourceComp, int destSeq, int destComp)
{
	for (int i = 0; i < sizeOfSerializedSequence; i++)
	{
		memoryInternalCompositionBuffer[destSeq][(destComp*sizeOfSerializedSequence) + i] = 
		memoryInternalCompositionBuffer[sourceSeq][(sourceComp*sizeOfSerializedSequence) + i];
	}
	
}

