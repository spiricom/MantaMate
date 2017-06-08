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
//#include "7Segment.h"

bool compositionMap[2][16];

CompositionAction compositionAction = CompositionRead;

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
void resetRangeStack(void);
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
	OptionSeqOnePitchTrigger,
	OptionSeqTwoPitchTrigger,
	OptionFullSplit,
	OptionNilOne,
	OptionNilTwo,
	OptionNilThree,
	OptionSeqLeft,
	OptionSeqRight,
	OptionPanelButtonTypeNil,
	OptionEndPattern = OptionSeqOnePitchTrigger,
	OptionEndMode = OptionNilOne,
	
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
	
	OptionSeqOnePitchTrigger, 
	OptionSeqTwoPitchTrigger, 
	OptionFullSplit,  
	OptionNilOne, 
	OptionNilTwo, 
	OptionNilThree,
	OptionSeqLeft, 
	OptionSeqRight
};

#define NUM_SEQ 2

tSequencer sequencer[NUM_SEQ];


uint8_t amberHexes[NUM_SEQ][MAX_STEPS];

int rangeNoteOn;
tNoteStack rangeStack;

int editNoteOn; 
int keyNoteOn;
int glideNoteOn;
int trigSelectOn;


tNoteStack editStack;
tNoteStack noteOnFrame;
tNoteStack noteOffFrame; // frames refreshed every 2 ms
tNoteStack noteOnStack; // all notes on at any point during runtime

uint8_t range_top = 15;
uint8_t range_bottom = 0;

#define sizeOfSerializedSequence  620 // increase this if the size of the serialized data gets larger (I shrank them to just slightly above the needed 611 because storing the memory internally takes up a lot of room -JS)
#define sizeOfBankOfSequences  sizeOfSerializedSequence*16
uint16_t encodeBuffer[sizeOfSerializedSequence]; 
uint16_t decodeBuffer[sizeOfSerializedSequence];
static uint16_t memoryInternalCompositionBuffer[NUM_SEQ][sizeOfBankOfSequences]; //9920 is 620 (number of bytes per sequence) * 16 (number of sequences that can be stored for each sequencer channel)


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

void initSHArrays(void)
{
	
}

void initSequencer(void)
{
	sequencer_mode = 1;
	
	compositionMap[SequencerOne][0] = true;
	compositionMap[SequencerTwo][0] = true;
	
	//memorySPIEraseBlock(preset_num); // DONT KEEP THIS
	
	
	initTimers();
	
	initSHArrays(); //what was this supposed to do? currently it does nothing. JS
	
	// Sequencer Modes
	pattern_type =				LeftRightRowUp;
	currentMantaSliderMode =	SliderModeOne;
	prevMantaSliderMode =		SliderModeOne;
	edit_vs_play =				PlayToggleMode; manta_set_LED_button(ButtonTopRight, Amber);
	currentFunctionButton =		ButtonTopLeft;
	key_vs_option =				KeyboardMode;
	playSubMode =				SeqMode; manta_set_LED_button(ButtonBottomRight, Off);
	
	tRampInit(&out00, 2000, 3, 1);
	tRampInit(&out02, 2000, 3, 1);
	tRampInit(&out10, 2000, 3, 1);
	tRampInit(&out11, 2000, 3, 1);
	tRampInit(&out12, 2000, 3, 1);
	tRampInit(&out13, 2000, 3, 1);
	tRampInit(&out20, 2000, 3, 1);
	tRampInit(&out21, 2000, 3, 1);
	tRampInit(&out22, 2000, 3, 1);
	tRampInit(&out23, 2000, 3, 1);
	
	tNoteStack_init(&editStack,		32);
	
	resetEditStack();
	
	tNoteStack_init(&noteOffFrame,	32);
	tNoteStack_init(&noteOnFrame,	32);
	tNoteStack_init(&rangeStack,	32);
	
	// Initialize the noteOnStack. :D !!!
	tNoteStack_init(&noteOnStack, 32); 
	// Glad I commented that - thanks @Josh Becker for the tip on appropriate commenting.
	
	setCurrentSequencer(SequencerOne);
	keyNoteOn = -1;
	glideNoteOn = -1;
	rangeNoteOn = -1;
	
	seq1PvT = PitchMode; seq2PvT = PitchMode;
	for (int i = 0; i < NUM_SEQ; i++)
	{		
		tSequencer_init(&sequencer[i], PitchMode, 32);
	}
	
	setSequencerLEDsFor(currentSequencer);
	
	setKeyboardLEDsFor(currentSequencer, -1);

	tc_start(tc3, TC3_CHANNEL);
	manta_send_LED();
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
			tSequencer_next(&sequencer[i]); // Move to next step, seq 1.
			
			curr = sequencer[i].currentStep;
			
			if (sequencer[i].stepGo)
			{
				if (sequencer[i].pitchOrTrigger == PitchMode)
				{
					dacSendPitchMode(i, curr);
				}
				else // TriggerMode
				{
					dacSendTriggerMode(i, curr);
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
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0))
		{
			newHexUIOn = i;
			newLowerHexSeq = 1;
			tNoteStack_add(&noteOnFrame,	i);
			tNoteStack_add(&noteOnStack, i);
		}
		
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			newHexUIOff = i;
			newReleaseLowerHexSeq = 1;
			tNoteStack_add(&noteOffFrame,	i);
			tNoteStack_remove(&noteOnStack, i);
			
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
	
	if (newReleaseLowerHexSeq)		releaseLowerHex(newHexUIOff);
	
	if (newLowerHexSeq)				touchLowerHex(newHexUIOn);
	
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
		uint8_t hex = 0;
		for (int i = 0; i < noteOffFrame.size; i++)
		{
			hex = noteOffFrame.notestack[i];			
			if (hex == editNoteOn)
			{	
				if (noteOnStack.size)	editNoteOn = noteOnStack.notestack[0];
				else					editNoteOn = -1;
				
			}
		}
	}
	else if (edit_vs_play == PlayToggleMode)
	{
		for (int i = 0; i < noteOffFrame.size; i++)
		{
			int hex = noteOffFrame.notestack[i];
			int step = hexUIToStep(hex);
			
			if (playSubMode == ArpMode)
			{
				tSequencer_toggleStep(&sequencer[currentSequencer],step);
				
				manta_set_LED_hex(hex, Off);
			} 
			else if (playSubMode == RangeMode)
			{
				tNoteStack_remove(&rangeStack, hexagon);
				
				if (hex == rangeNoteOn)
				{
					rangeNoteOn = -1;
				}
			}
			else // SeqMode
			{
			
			}
		}
	}
	
	tNoteStack_clear(&noteOffFrame);
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
		for (int comp = 0; comp < 16; comp++)
		{
			if (compositionMap[seq][comp])
			{
				if (compositionAction == CompositionRead)	manta_set_LED_hex(seq * 16 + comp, Amber);
				else /* CompositionWrite */					manta_set_LED_hex(seq * 16 + comp, Red);
			}
			
		}
	}
}
MantaSequencer whichSeq_test;
int whichComp_test;
void touchLowerHex(uint8_t hexagon)
{
	if (key_vs_option == OptionMode)
	{
		MantaSequencer whichSeq = (hexagon < 16) ? SequencerOne : SequencerTwo;
		int whichComp = (hexagon < 16) ? hexagon : (hexagon - 16);
		
		whichSeq_test = whichSeq;
		whichComp_test = whichComp;
		
		if (compositionAction == CompositionRead)
		{
			manta_set_LED_hex(hexagon, Red);
			
			if (compositionMap[whichSeq][whichComp])
			{
				memoryInternalReadSequencer(whichSeq, whichComp, &decodeBuffer);
				tSequencer_decode(&sequencer[whichSeq], &decodeBuffer);
			}

		}
		else // CompositionWrite 
		{
			manta_set_LED_hex(hexagon, Amber);
			
			compositionMap[whichSeq][whichComp] = true;
			tSequencer_encode(&sequencer[whichSeq], &encodeBuffer);
			memoryInternalWriteSequencer(whichSeq, whichComp, &encodeBuffer);
		}
	}
	else
	{
		if ((full_vs_split == SplitMode) && (edit_vs_play == TrigToggleMode) && ((hexagon < 16 && trigSelectOn >= 12) || (hexagon >= 16 && trigSelectOn < 4)))
		{
			tNoteStack_clear(&noteOnFrame);
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
		
		int step = 0;
		
		for (int i = 0; i < noteOnFrame.size; i++)
		{
			hexagon = noteOnFrame.notestack[i];
			step = hexUIToStep(hexagon);
			
			if (edit_vs_play == EditMode)
			{
				if (editNoteOn >= 0)
				{
					// If the first hex added is still touched, add new hex to edit stack.
					if (tNoteStack_contains(&editStack, hexagon) == -1)
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
					
					editNoteOn = tNoteStack_first(&editStack);
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
				else if (playSubMode == RangeMode)
				{
					if (rangeNoteOn >= 0)
					{
						// If the first hex added is still touched, add new hex to edit stack.
						// (contains returns index of hex in notestack if hex is found)
						
						clearSequencer(currentSequencer);
						
						if (tNoteStack_contains(&rangeStack, hexagon) < 0)
						{
							tNoteStack_add(&rangeStack, hexagon);
							manta_set_LED_hex(hexagon, Red);
						}

						first = 32;
						last = -1;
						int rangeHex = -1;
						for (int i = 0; i < rangeStack.size; i++)
						{
							rangeHex = tSequencer_getStepFromHex(&sequencer[currentSequencer], rangeStack.notestack[i]);
							
							if (rangeHex >= 0)
							{
								if (rangeHex < first)
								{
									first = rangeHex;
								}
								
								if (rangeHex > last)
								{
									last = rangeHex;
								}
								
							}
						}
						
						for (int i = first; i <= last; i++)
						{

							int nextStep = tSequencer_getHexFromStep(&sequencer[currentSequencer], i);
							tSequencer_toggleStep(&sequencer[currentSequencer], hexUIToStep(nextStep));

							manta_set_LED_hex(nextStep, AmberOn);
						}
						
					}
					else
					{
						resetRangeStack();
						setSequencerLEDsFor(sequencerToSet);
						
						rangeNoteOn = tNoteStack_first(&rangeStack);
						manta_set_LED_hex(rangeNoteOn,Red);
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
	}
	
	
	
	tNoteStack_clear(&noteOnFrame);
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
		
		manta_set_LED_button(ButtonTopRight, Off);
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

	if (key_vs_option == OptionMode)
	{
		
		if (optionModeButtons[hexagon-MAX_STEPS] == OptionNilTwo)
		{
			manta_set_LED_hex(MAX_STEPS+12, Off);
			return;
		}
		
		if (optionModeButtons[hexagon-MAX_STEPS] == OptionNilThree)
		{
			manta_set_LED_hex(MAX_STEPS+13, Off);
			return;
		}
	}
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
		
		if (whichOptionType < OptionEndPattern )
		{
			tSequencer_setPattern(&sequencer[currentSequencer], whichOptionType);
				
			prev_pattern_hex = current_pattern_hex;
			current_pattern_hex = whichHex;
		}
		else if ((whichOptionType >= OptionSeqOnePitchTrigger) && (whichOptionType < OptionEndMode))
		{
			resetEditStack();
			
			prev_option_hex = current_option_hex;
			current_option_hex = whichHex;
			
			currentHexUI = 0;
				
			currentUpperHexUI = 0;
			
			if (optionModeButtons[whichHex] == OptionSeqOnePitchTrigger)
			{
				if (sequencer[SequencerOne].pitchOrTrigger == PitchMode)
				{
					tSequencer_init(&sequencer[SequencerOne], TriggerMode, MAX_STEPS); seq1PvT = TriggerMode;
				}
				else if (sequencer[SequencerOne].pitchOrTrigger == TriggerMode)
				{
					tSequencer_init(&sequencer[SequencerOne], PitchMode, MAX_STEPS); seq1PvT = PitchMode;
				}
				setCurrentSequencer(SequencerOne);
			}
			else if (whichOptionType == OptionSeqTwoPitchTrigger)
			{
				if (sequencer[SequencerTwo].pitchOrTrigger == PitchMode)
				{
					tSequencer_init(&sequencer[SequencerTwo], TriggerMode, MAX_STEPS); seq2PvT = TriggerMode;
				}
				else if (sequencer[SequencerTwo].pitchOrTrigger == TriggerMode)
				{
					tSequencer_init(&sequencer[SequencerTwo], PitchMode, MAX_STEPS); seq2PvT = PitchMode;
				}
				setCurrentSequencer(SequencerTwo);
			}
			else if (whichOptionType == OptionFullSplit)
			{
				if (full_vs_split == FullMode)			
				{
					full_vs_split = SplitMode;
				}
				else if (full_vs_split == SplitMode)	
				{
					full_vs_split = FullMode;
				}
				
				setCurrentSequencer(SequencerOne);
			} 
			else
			{
				// No other mode, sorry.
			}
			
			if (full_vs_split == FullMode)
			{
				setSequencerLEDsFor(currentSequencer);
			}
			else //SplitMode
			{
				setSequencerLEDsFor(SequencerOne);
				setSequencerLEDsFor(SequencerTwo);
			}
				
			setOptionLEDs();
		
		}
		else if ((whichOptionType == OptionSeqLeft) || (whichOptionType == OptionSeqRight))
		{
			if (sequencer[currentSequencer].pitchOrTrigger == PitchMode)
			{
				if (full_vs_split == FullMode)
				{
					prev_panel_hex = current_panel_hex;
					current_panel_hex = whichHex;
					
					setCurrentSequencer(whichOptionType - OptionSeqLeft);
					setSequencerLEDsFor(currentSequencer);
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
		/*
		else if (whichOptionType == OptionNilTwo)
		{
			// WRITE
			
			
			
			
		}
		else if (whichOptionType == OptionNilThree)
		{
			// READ
			
		}
		*/
		

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
	if (key_vs_option == OptionMode) return;
}

void releaseTopRightButton(void)
{
	if (key_vs_option == OptionMode) return;
	
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

// ~ ~ ~ ~ BOTTOM LEFT BUTTON ~ ~ ~ ~ //
void touchBottomLeftButton(void)
{
	key_vs_option = OptionMode;
	
	allUIStepsOff(currentSequencer);
	setOptionLEDs();
	
	manta_set_LED_button(ButtonBottomLeft, Red);
	
}

void releaseBottomLeftButton(void)
{
	key_vs_option = KeyboardMode;
	
	if (edit_vs_play == TrigToggleMode)		setKeyboardLEDsFor(currentSequencer, 0);
	else /* PlayToggleMode or EditMode */	setKeyboardLEDsFor(currentSequencer, -1);
	
	setSequencerLEDsFor(currentSequencer);

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
			playSubMode = RangeMode;
			resetRangeStack();
			manta_set_LED_button(ButtonBottomRight, Amber);
		}
		else if (playSubMode == RangeMode)
		{
			playSubMode = ArpMode;
			
			tSequencer_clearSteps(&sequencer[SequencerOne]);
			tSequencer_clearSteps(&sequencer[SequencerTwo]);
			
			setSequencerLEDsFor(currentSequencer);
			
			manta_set_LED_button(ButtonBottomRight, Red);
		} else // ArpMode
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

		if (tNoteStack_contains(&editStack, uiHexCurrentStep) != -1)
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
		
		if (tNoteStack_contains(&editStack,uiHexCurrentStep) != -1)
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
	OptionPanelButtonType modeButton = OptionPanelButtonTypeNil;
	uint8_t modeHex = 64;
	for (int i = 0; i < 16; i++)
	{
		modeButton = optionModeButtons[i];
		modeHex = modeButton + MAX_STEPS;
		if ((modeButton == OptionNilOne) || (modeButton == OptionNilTwo) || (modeButton == OptionNilThree))
		{
			manta_set_LED_hex(modeHex, Off);
		}
		else if ((modeButton >= OptionSeqOnePitchTrigger) && (modeButton < OptionEndMode))
		{
			manta_set_LED_hex(modeHex, Amber);
		}
		else if (modeButton < OptionEndPattern)
		{
			manta_set_LED_hex(modeHex, (i == sequencer[currentSequencer].pattern) ? Red : Amber);
		}
		else if ((modeButton == OptionSeqLeft) || (modeButton == OptionSeqRight))
		{
			if (full_vs_split == FullMode && edit_vs_play != TrigToggleMode)
			{
				manta_set_LED_hex(modeHex, ((modeButton - OptionSeqLeft) == currentSequencer) ? Red : Amber);
			}
			else
			{
				manta_set_LED_hex(i+MAX_STEPS, Off);
			}	
		}
	}
	
    // Set Pitch / Trigger and Full / Split indicator LEDs
	manta_set_LED_hex(MAX_STEPS + 8, (sequencer[SequencerOne].pitchOrTrigger == PitchMode) ? Amber : Red); 

	manta_set_LED_hex(MAX_STEPS + 9, (sequencer[SequencerTwo].pitchOrTrigger == PitchMode) ? Amber : Red);

	manta_set_LED_hex(MAX_STEPS + 10, (full_vs_split == FullMode) ? Amber : Red);
	
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
	int offset = 2;
	if (seq ==SequencerOne)
	{
		offset = 0;
	}
	
	if (sequencer[seq].step[step].note)
	{
		// Configure PitchGlide
		tRamp* glide = &out00;
		if (seq == SequencerTwo) glide = &out02;
		
		uint16_t glideTime =  sequencer[seq].step[step].pglide >> 3;
		if (glideTime < 5) glideTime = 5;
		
		tRampSetDest(glide, (float)get16BitPitch(seq,step) / UINT16_MAX); 
		tRampSetTime(glide, glideTime);

		// Configure CVGlide
		glideTime =  sequencer[seq].step[step].cvglide >> 3;
		if (glideTime < 5) glideTime = 5;
		
		if (seq == SequencerOne) 
		{
			tRampSetTime(&out20, glideTime);
			tRampSetDest(&out20, (float) sequencer[SequencerOne].step[step].cv3);
			
			tRampSetTime(&out21, glideTime);
			tRampSetDest(&out21, (float) sequencer[SequencerOne].step[step].cv4);
			
			tRampSetTime(&out10, glideTime);
			tRampSetDest(&out10, (float) sequencer[SequencerOne].step[step].cv1);
					
			tRampSetTime(&out11, glideTime);
			tRampSetDest(&out11, (float) sequencer[SequencerOne].step[step].cv2);
			
		} 
		else //SequencerTwo
		{
			tRampSetTime(&out22, glideTime);
			tRampSetDest(&out22, (float) sequencer[SequencerTwo].step[step].cv3);
			
			tRampSetTime(&out23, glideTime);
			tRampSetDest(&out23, (float) sequencer[SequencerTwo].step[step].cv4);
			
			tRampSetTime(&out12, glideTime);
			tRampSetDest(&out12, (float) sequencer[SequencerTwo].step[step].cv1);
			
			tRampSetTime(&out13, glideTime);
			tRampSetDest(&out13, (float) sequencer[SequencerTwo].step[step].cv2);
			
		}
		
		
		// Send Trigger
		DAC16Send(offset+1, 65535);
	}
}

void dacSendTriggerMode(MantaSequencer seq, uint8_t step)
{

	int offset = 0;
	if (seq == SequencerTwo) offset = 2;
	
	// Configure CVGlide
	uint16_t glideTime =  sequencer[seq].step[step].cvglide >> 3;
	if (glideTime < 5) glideTime = 5;
	
	if (seq == SequencerOne)
	{
		tRampSetTime(&out20, glideTime);
		tRampSetDest(&out20, (float) sequencer[SequencerOne].step[step].cv1);
		
		tRampSetTime(&out21, glideTime);
		tRampSetDest(&out21, (float) sequencer[SequencerOne].step[step].cv2);
		
	}
	else //SequencerTwo
	{
		tRampSetTime(&out22, glideTime);
		tRampSetDest(&out22, (float) sequencer[SequencerTwo].step[step].cv1);
		
		tRampSetTime(&out23, glideTime);
		tRampSetDest(&out23, (float) sequencer[SequencerTwo].step[step].cv2);
	}
	
	// Trigger 1, Trigger 2, Trigger 3, Trigger 4
	dacsend(offset+0, 0, sequencer[seq].step[step].on[0] * 4095);
	dacsend(offset+1, 0, sequencer[seq].step[step].on[1] * 4095);
	dacsend(offset+0, 1, sequencer[seq].step[step].on[2] * 4095);
	dacsend(offset+1, 1, sequencer[seq].step[step].on[3] * 4095);
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

void resetRangeStack(void)
{
	tNoteStack_clear(&rangeStack);
	rangeNoteOn = -1;
	tNoteStack_add(&rangeStack, currentHexUI);
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