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
MantaInstrument copyWhichInst = InstrumentOne;
int copyWhichComp = -1;

PanelSwitch panelSwitch[NUM_PANEL_MOVES] =
{
	PanelLeft,
	PanelRight
};


// OUTPUT
void dacSendPitchMode	(MantaInstrument, uint8_t step);
void dacSendTriggerMode	(MantaInstrument, uint8_t step);

// LEDs
void setPanelSelectLEDs		(void);
void setSliderLEDsFor		(MantaInstrument, int note);
void setKeyboardLEDsFor		(MantaInstrument, int note);
void setOptionLEDs			(void);
void setCompositionLEDs     (void);
void setKeyboardConfigLEDs	(void);
void setSequencerLEDsFor	(MantaInstrument);
void setSequencerLEDs		(void);
void setTriggerPanelLEDsFor	(MantaInstrument, TriggerPanel panel);
void resetSliderMode		(void);

// UTILITIES
void seqwait(void);
uint32_t get16BitPitch(MantaInstrument, uint8_t step);
void setCurrentInstrument(MantaInstrument);
void setParameterForEditStackSteps(MantaInstrument, StepParameterType param, uint16_t value);
void setParameterForStep(MantaInstrument, uint8_t step, StepParameterType param, uint16_t value);
uint16_t getParameterFromStep(MantaInstrument, uint8_t step, StepParameterType param);
void resetEditStack(void);
uint8_t hexUIToStep(uint8_t hexagon);
uint8_t stepToHexUI(MantaInstrument, uint8_t noteIn);
void downOctaveForEditStackSteps(MantaInstrument);
void upOctaveForEditStackSteps(MantaInstrument);

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
	OptionKeyboard,
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
	OptionKeyboard,
	OptionNil, 
	OptionFullSplit,
	OptionNil,
	OptionLeft, 
	OptionRight
};


uint8_t amberHexes[NUM_INST][MAX_STEPS];

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



uint16_t encodeBuffer[NUM_INST][sizeOfSerializedSequence]; 
uint16_t decodeBuffer[NUM_INST][sizeOfSerializedSequence];
uint16_t memoryInternalCompositionBuffer[NUM_INST][sizeOfBankOfSequences]; //9920 is 620 (number of bytes per sequence) * 16 (number of sequences that can be stored for each sequencer channel)

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

void setCurrentInstrument(MantaInstrument inst)
{
	if (currentInstrument != inst)
	{
		editNoteOn = -1;
		currentInstrument = inst;
	}
	
}

bool blinkToggle;
void blink(void)
{
	blinkToggle = (blinkToggle == true) ? false : true;		   
	
	if (key_vs_option == OptionMode && compositionAction == CompositionCopy)
	{
		for (int inst = 0; inst < 2; inst++)
		{
			for (int comp = 0; comp < NUM_COMP; comp++)
			{
				if (compositionMap[inst][comp])
				{
					manta_set_LED_hex(inst * 16 + comp, blinkToggle ? AmberOn : AmberOff);
				}
			}
		}
		
		if (copyWhichInst != InstrumentNil && copyWhichComp != -1) 
			manta_set_LED_hex(copyWhichInst * 16 + copyWhichComp, blinkToggle ? Red : Off);
	}
}

void initSequencer(void)
{
	Write7Seg(preset_num); // shouldn't have to do this here, but there's a bug that writes garbage to the 7Seg when plugging in a Manta, and this seems to fix it
	initTimers();
	
	// Sequencer Modes
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
	
	setCurrentInstrument(InstrumentOne);
	keyNoteOn = -1;
	glideNoteOn = -1;
	
	
	if (preset_num == 0)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			if (preset_num == 0)
			{
				//if preset_num == 0, then we are trying to initialize the blank default sequencer
				manta[i].type = SequencerInstrument;
				tSequencer_init(&manta[i].sequencer, PitchMode, 32);
				
				tSequencer_encode(&manta[i].sequencer, encodeBuffer);
				memoryInternalWriteSequencer(i, 0, encodeBuffer);
				
				compositionMap[i][0] = true;
				currentComp[i] = 0;
			}
			else
			{   //otherwise we are loading a saved preset, so we need to prepare that data properly
				tSequencer_init(&manta[i].sequencer, PitchMode, 32);
				tSequencer_encode(&manta[i].sequencer, encodeBuffer);
				memoryInternalWriteSequencer(i, 0, encodeBuffer);
				compositionMap[i][0] = true;
				currentComp[i] = 0;
				tSequencer_decode(&manta[i].sequencer, decodeBuffer);
			}
		}
	}
	
	setSequencerLEDsFor(currentInstrument);
	
	setKeyboardLEDsFor(currentInstrument, -1);

	tc_start(tc3, TC3_CHANNEL);
	manta_send_LED();

}

void sequencerStep(MantaInstrument inst)
{
	
	int offset,cstep,curr;
	
	tSequencer* sequencer = &manta[inst].sequencer;
		
	offset = inst * 2;
		
	cstep = sequencer->currentStep;
		
	sequencer->lengthCounter += 1;
		
	if (sequencer->lengthCounter >= sequencer->step[cstep].length)
	{
		tSequencer_next(sequencer); // Move to next step, seq 1.
			
		curr = sequencer->currentStep;
			
		if (sequencer->stepGo)
		{
			if (sequencer->pitchOrTrigger == PitchMode)
			{
				dacSendPitchMode(inst, curr);
			}
			else // TriggerMode
			{
				dacSendTriggerMode(inst, curr);
			}

			// Start sequencer 1 and 2 timers: tc1, tc2.
			// Timer callbacks tc1_irq and tc2_irq (in main.c) set dac trigger outputs low on every step.
			tc_start(tc1, TC1_CHANNEL);
		}
		sequencer->lengthCounter = 0;
	}
	
	uiStep(inst);

}

MantaButton lastFunctionButton;

void processHexTouch(void)
{
	uint8_t newHexUIOn = 0;
	uint8_t newHexUIOff = 0;
	uint8_t newUpperHexUI = 0;
	MantaInstrumentType type = manta[currentInstrument].type;

	//check the sequencer step hexagons
	for (int i = 0; i < 48; i++)
	{
		// RELEASE
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			if (key_vs_option == OptionMode)
			{
				if (i < MAX_STEPS)	releaseLowerHexOptionMode(i);
				else				releaseUpperHexOptionMode(i);
			}
			else
			{
				if (type == SequencerInstrument)
				{
					if (i < MAX_STEPS) //Lower hex
					{
						newHexUIOff = i;
						newReleaseLowerHexSeq = 1;
					
						tNoteStack_remove(&noteOnStack, i);
					
						releaseLowerHex(newHexUIOff);
					}
					else // Upper hex
					{
						newUpperHexUI = i;
						newReleaseUpperHexSeq = 1;
					
						releaseUpperHex(newUpperHexUI);
					}
				}
				else
				{
					releaseLowerHexKey(i);
				}
			}
			
			
			
		}
		
		// TOUCH
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0)) // TOUCH
		{
			
			if (key_vs_option == OptionMode) 
			{
				if (i < MAX_STEPS)	touchLowerHexOptionMode(i);
				else				touchUpperHexOptionMode(i);
			}
			else
			{
				if (type == SequencerInstrument)
				{
					if (i < MAX_STEPS) //Lower hex
					{
						newHexUIOn = i;
						newLowerHexSeq = 1;
						
						tNoteStack_add(&noteOnStack, i);
						
						touchLowerHex(newHexUIOn);
					}
					else // Upper hex
					{
						//an upper hexagon was just pressed
						newUpperHexUI = i;
						newUpperHexSeq = 1;
						
						touchUpperHex(newUpperHexUI);
					}
				}
				else
				{
					touchLowerHexKey(i, butt_states[i]);
				}
			}
			
			
			

		}
		
		pastbutt_states[i] = butt_states[i];
	}
	
	for (int i = 0; i < 4; i++)
	{
		if ((func_button_states[i] > 0) && (past_func_button_states[i] <= 0))
		{
			//a round function button was just pressed
			resetSliderMode();
			
			if (i == ButtonTopLeft)				touchTopLeftButton();
			else if (i == ButtonTopRight)		touchTopRightButton();
			else if (i == ButtonBottomLeft)		touchBottomLeftButton();
			else if (i == ButtonBottomRight)	touchBottomRightButton();
		}
		
		if ((func_button_states[i] <= 0) && (past_func_button_states[i] > 0))
		{
			if (i == ButtonTopLeft)				releaseTopLeftButton();
			else if (i == ButtonTopRight)		releaseTopRightButton();
			else if (i == ButtonBottomLeft)		releaseBottomLeftButton();
			else if (i == ButtonBottomRight)	releaseBottomRightButton();
		}
		
		past_func_button_states[i] = func_button_states[i];
	}		
	
}

void releaseLowerHexOptionMode(uint8_t hexagon)
{
	if (manta[currentInstrument].type == SequencerInstrument)	setCompositionLEDs();
	else														setKeyboardConfigLEDs();
}

void releaseLowerHex(uint8_t hexagon)
{
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
			tSequencer_toggleStep(&manta[currentInstrument].sequencer,step);
				
			manta_set_LED_hex(hexagon, Off);
		} 
		else // SeqMode
		{
			
		}
		
	}
	newReleaseLowerHexSeq = 0;
}

void clearSequencer(MantaInstrument inst)
{
	tSequencer_clearSteps(&manta[inst].sequencer);
	setSequencerLEDsFor(inst);
}

int first, last;

void setKeyboardConfigLEDs	(void)
{
	for (int i = 0; i < 32; i++) manta_set_LED_hex(i, Off);
	
}

void setCompositionLEDs(void)
{
	for (int inst = 0 ; inst < 2; inst++)
	{
		if (manta[inst].type == SequencerInstrument)
		{
			for (int comp = 0; comp < NUM_COMP; comp++)
			{
				if (compositionMap[inst][comp])
				{
					if (compositionAction == CompositionRead)		manta_set_LED_hex(inst * 16 + comp, (currentComp[inst] == comp) ? Red : Amber);
					else if (compositionAction == CompositionWrite)	manta_set_LED_hex(inst * 16 + comp, Red);
				}
				
			}
			
			manta_set_LED_hex(14 + 16*inst, Amber);
			manta_set_LED_hex(15 + 16*inst, Red);
		}
	
	}
	
}

void touchLowerHexOptionMode(uint8_t hexagon)
{
	if (key_vs_option == OptionMode)
	{
		MantaInstrument whichInst = (hexagon < 16) ? InstrumentOne : InstrumentTwo;
		int whichComp = (hexagon < 16) ? hexagon : (hexagon - 16);
		
		if (whichComp == 14)
		{
			
			manta_set_LED_hex(whichInst * 16 + 14, Red);
		}
		else if (whichComp == 15)
		{
			tSequencer_init(&manta[whichInst].sequencer, manta[whichInst].sequencer.pitchOrTrigger, 32);
			
			manta_set_LED_hex(whichInst * 16 + 15, Amber);
		}
		else
		{
			if (compositionAction == CompositionRead)
			{
				if (compositionMap[whichInst][whichComp])
				{
					memoryInternalReadSequencer(whichInst, whichComp, decodeBuffer);
					tSequencer_decode(&manta[whichInst].sequencer, decodeBuffer);
					
					currentComp[whichInst] = whichComp;
					
					setCurrentInstrument(whichInst);
					
					setOptionLEDs();
				}

			}
			else if (compositionAction == CompositionWrite) // CompositionWrite
			{
				manta_set_LED_hex(hexagon, Amber);
				
				compositionMap[whichInst][whichComp] = true;
				tSequencer_encode(&manta[whichInst].sequencer, encodeBuffer);
				memoryInternalWriteSequencer(whichInst, whichComp, encodeBuffer);
			}
			else if (compositionAction == CompositionCopy)
			{
				if (copyStage == 0)
				{
					// this determines sequence to be copied
					if (compositionMap[whichInst][whichComp])
					{
						copyWhichInst = whichInst;
						copyWhichComp = whichComp;
						
						copyStage = 1;
					}
				}
				else if (copyStage == 1)
				{
					// any hex pressed while still in copy mode is written with sequencer from copystage 1
					if (!(whichInst == copyWhichInst && whichComp == copyWhichComp)) // if not the same step, copy
					{
						manta_set_LED_hex(hexagon, Red);
						compositionMap[whichInst][whichComp] = true;
						memoryInternalCopySequencer(copyWhichInst, copyWhichComp, whichInst, whichComp);
					}
				}
				
			}
			
		}

	}
}


void touchLowerHex(uint8_t hexagon)
{
	
	if (manta[currentInstrument].type == SequencerInstrument)
	{
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

		MantaInstrument instrumentToSet = currentInstrument;
		if (full_vs_split == SplitMode)
		{
			if (hexagon < 16)	setCurrentInstrument(InstrumentOne);
			else				setCurrentInstrument(InstrumentTwo);
			
			if (instrumentToSet != currentInstrument)
			{
				resetEditStack();
				if (edit_vs_play != TrigToggleMode)
				{
					setSequencerLEDsFor(InstrumentOne);
					setSequencerLEDsFor(InstrumentTwo);
				}
				else
				{
					setTriggerPanelLEDsFor(InstrumentOne,currentPanel[InstrumentOne]);
					setTriggerPanelLEDsFor(InstrumentTwo,currentPanel[InstrumentTwo]);
				}
			}
		}
		
		
		uint8_t uiHexCurrentStep = stepToHexUI(currentInstrument, manta[currentInstrument].sequencer.currentStep);
		uint8_t uiHexPrevStep = stepToHexUI(currentInstrument, manta[currentInstrument].sequencer.prevStep);
		
		int step = hexUIToStep(hexagon);
		
		tSequencer* sequencer = &manta[currentInstrument].sequencer;
		
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

					if (amberHexes[currentInstrument][hexagon] == 1)	manta_set_LED_hex(hexagon, Amber);
					else												manta_set_LED_hex(hexagon, Off);
				}
			}
			else
			{
				resetEditStack();
				setSequencerLEDsFor(instrumentToSet);
				
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
				if (tSequencer_toggleStep(sequencer, step))
				{
					manta_set_LED_hex(hexagon, AmberOn);
					amberHexes[currentInstrument][hexagon] = 1;
				}
				else
				{
					manta_set_LED_hex(hexagon, AmberOff);
					amberHexes[currentInstrument][hexagon] = 0;
					if (hexagon == uiHexCurrentStep)
					{
						manta_set_LED_hex(hexagon, RedOff);
					}
				}
			}
			else // ArpMode
			{
				tSequencer_toggleStep(sequencer, step);
				manta_set_LED_hex(hexagon, AmberOn);
			}
		}
		else // TrigToggleMode
		{
			tNoteStack_remove(&editStack, prevHexUI);
			tNoteStack_add(&editStack,currentHexUI);
			
			int currPanel = currentPanel[currentInstrument];
			
			int cont = 1;
			
			if (cont)
			{
				if (sequencer->step[step].on[currPanel])
				{
					sequencer->step[step].on[currPanel] = 0;
					manta_set_LED_hex(currentHexUI, RedOff);
				}
				else
				{
					sequencer->step[step].on[currPanel] = 1;
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
					setKeyboardLEDsFor(currentInstrument, -1);
				}
				else
				{
					setKeyboardLEDsFor(currentInstrument, hexUIToStep(hexagon));
				}
			}
		}
		else
		{
			setOptionLEDs();
		}
		
		if (editStack.size > 1)
		{
			setSliderLEDsFor(currentInstrument, -1);
		}
		else
		{
			setSliderLEDsFor(currentInstrument, hexUIToStep(hexagon));
		}
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
			if (trigSelectOn >= 0 && trigSelectOn < 4)	setTriggerPanelLEDsFor(InstrumentOne,currentPanel[InstrumentOne]);
			else										setTriggerPanelLEDsFor(InstrumentTwo,currentPanel[InstrumentTwo]);
			
			manta_set_LED_hex(stepToHexUI(InstrumentOne, manta[InstrumentOne].sequencer.currentStep), Amber);
			manta_set_LED_hex(stepToHexUI(InstrumentTwo, manta[InstrumentTwo].sequencer.currentStep), Amber);
		}
		else // FullMode
		{
			setTriggerPanelLEDsFor(currentInstrument,currentPanel[currentInstrument]);
			
			manta_set_LED_hex(stepToHexUI(currentInstrument, manta[currentInstrument].sequencer.currentStep), Amber);
			
		}
		
	}
	else if (mode == PlayToggleMode)
	{
		if (full_vs_split == SplitMode)
		{
			setSequencerLEDsFor(InstrumentOne);
			setSequencerLEDsFor(InstrumentTwo);
		}
		else // FullMode
		{
			setSequencerLEDsFor(currentInstrument);
		}
		manta_set_LED_button(ButtonTopRight, Amber);
	}
	else // EditMode
	{
		int currHex = tNoteStack_first(&editStack);
		
		if (currHex != manta[currentInstrument].sequencer.currentStep)
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

void releaseUpperHexOptionMode(uint8_t hexagon)
{
	
}

void releaseUpperHex(uint8_t hexagon)
{
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	
	if (sequencer->pitchOrTrigger == PitchMode)
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
					setSliderLEDsFor(currentInstrument, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentInstrument, -1);
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
					setSliderLEDsFor(currentInstrument, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentInstrument, -1);
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
					setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
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
					setSliderLEDsFor(currentInstrument, tNoteStack_first(&editStack));
				}
				else
				{
					setSliderLEDsFor(currentInstrument, -1);
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

void allUIStepsOff(MantaInstrument inst)
{
	int hexUI = 0;
	for (int i = 0; i < manta[inst].sequencer.maxLength; i++)
	{
		hexUI = stepToHexUI(inst,i);
		
		manta_set_LED_hex(hexUI ,Off);
	}
}

void touchUpperHexOptionMode(uint8_t hexagon)
{
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	
	uint8_t whichHex = hexagon - MAX_STEPS;
	
	OptionPanelButtonType whichOptionType = optionModeButtons[whichHex];
	
	if (whichOptionType <= OptionPatternEight)
	{
		tSequencer_setPattern(sequencer, whichOptionType);
		
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

		if (manta[currentInstrument].type != SequencerInstrument)
		{
			manta[currentInstrument].type = SequencerInstrument;
			tSequencer_init(sequencer, PitchMode, MAX_STEPS);
		}
		else if (sequencer->pitchOrTrigger != PitchMode)
		{
			tSequencer_init(sequencer, PitchMode, MAX_STEPS);
		}
		
	}
	else if (whichOptionType == OptionTrigger)
	{
		resetEditStack();
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;

		if (manta[currentInstrument].type != SequencerInstrument)
		{
			manta[currentInstrument].type = SequencerInstrument;
			tSequencer_init(sequencer, TriggerMode, MAX_STEPS);
		}
		else if (sequencer->pitchOrTrigger != TriggerMode)
		{
			tSequencer_init(sequencer, TriggerMode, MAX_STEPS);
		}
		
	}
	else if (whichOptionType == OptionKeyboard)
	{
		resetEditStack();
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;
		
		if (manta[currentInstrument].type != KeyboardInstrument)
		{
			manta[currentInstrument].type = KeyboardInstrument;
			
			tKeyboard_init(keyboard, 1);
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
		
		setCurrentInstrument(InstrumentOne);
	}
	else if (whichOptionType == OptionRight)
	{
		prev_panel_hex = current_panel_hex;
		current_panel_hex = whichHex;
		
		setCurrentInstrument(InstrumentTwo);
	}
	
	setOptionLEDs();
	
	//set memory variables
	newUpperHexSeq = 0;
}

void touchUpperHex(uint8_t hexagon)
{
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	

	if (key_vs_option == KeyboardMode)
	{
		if (sequencer->pitchOrTrigger == PitchMode)
		{
			int upperHexType = keyboard_pattern[hexagon-MAX_STEPS];
				
			if (upperHexType == KeyboardPanelRest)
			{
				// handle case when more than one note in editStack (check if they all have same parameter value
				// need to be able to set multiple notes to rest too
				int step = hexUIToStep(tNoteStack_first(&editStack));
					
				if (!(getParameterFromStep(currentInstrument, step, Note)))	setParameterForEditStackSteps(currentInstrument,Note,1);
				else														setParameterForEditStackSteps(currentInstrument,Note,0);
			}
			else if (upperHexType == KeyboardPanelOctaveDown)
			{
				// down an octave
				tSequencer_downOctave(sequencer);
				downOctaveForEditStackSteps(currentInstrument);
				
				// Temporarily set slider LEDs to display current octave.
				if (editStack.size > 1) setSliderLEDsFor(currentInstrument, -1);
				else					manta_set_LED_slider(SliderOne, sequencer->step[hexUIToStep(editStack.notestack[0])].octave + 1);
				
				
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
				tSequencer_upOctave(sequencer);
				upOctaveForEditStackSteps(currentInstrument);
				// TODO: Only set this LED if top left function button is red... Unsure how to do that ATM - JSB
				
				// Temporarily set slider LEDs to display current octave.
				if (editStack.size > 1) setSliderLEDsFor(currentInstrument, -1);
				else					manta_set_LED_slider(SliderOne, sequencer->step[hexUIToStep(editStack.notestack[0])].octave + 1);
				
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
						setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentInstrument, -1);
					}
				}
				
			}
			else //KeyboardPanelKeyX
			{
				if (keyNoteOn == -1)
				{
					keyNoteOn = hexagon;
				
					// In this case, upperHexType is pitch on keyboard.
					setParameterForEditStackSteps(currentInstrument, Note, 1);
					
					setParameterForEditStackSteps(currentInstrument, Pitch, upperHexType);

					setParameterForEditStackSteps(currentInstrument, Octave, sequencer->octave);
					
					setParameterForEditStackSteps(currentInstrument, KbdHex, currentUpperHexUI);

					manta_set_LED_hex(hexagon, Red);
				
				
					// Enter SliderModePitch 
					prevMantaSliderMode = currentMantaSliderMode;
					currentMantaSliderMode = SliderModePitch;
				
					if (editStack.size <= 1)
					{
						setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentInstrument, -1);
					}
				}
			}
				
			int cStep = sequencer->currentStep;
			if (tNoteStack_contains(&editStack,stepToHexUI(currentInstrument,cStep)) != -1)
			{
				//comment this out if we don't want immediate DAC update, but only update at the beginning of a clock
				DAC16Send(2 * currentInstrument, get16BitPitch(currentInstrument, cStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
				

			setKeyboardLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
		}
		else // TriggerMode
		{
			int whichUpperHex = currentUpperHexUI - MAX_STEPS;
			int whichTrigPanel = trigger_pattern[whichUpperHex];
			
			MantaInstrument whichInst = currentInstrument;
			int whichPanel = 0;
			int cont = 0;
			
			if (manta[InstrumentOne].type == SequencerInstrument && whichTrigPanel == S1TrigPanelSelect)
			{
				if (manta[InstrumentOne].sequencer.pitchOrTrigger == TriggerMode)
				{
					whichInst = InstrumentOne;
					whichPanel = whichUpperHex;
					
					if (edit_vs_play == PlayToggleMode)
					{
						trigSelectOn = whichUpperHex;
						switchToMode(TrigToggleMode);
					}
					
					cont= 1;
				}
			}
			else if (manta[InstrumentTwo].type == SequencerInstrument && whichTrigPanel == S2TrigPanelSelect)
			{
				if (manta[InstrumentTwo].sequencer.pitchOrTrigger == TriggerMode)
				{
					whichInst = InstrumentTwo;
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
						setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
					}
					else
					{
						setSliderLEDsFor(currentInstrument, -1);
					}
				}
			}
			
			if (cont) //dumb method
			{
				if (whichInst != currentInstrument)
				{
					setCurrentInstrument(whichInst);
				
					if (edit_vs_play != TrigToggleMode)
					{
						setKeyboardLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
						setSequencerLEDsFor(currentInstrument);
					}
				}
			
				int uiOffset = MAX_STEPS;
			
				if (whichInst == InstrumentTwo) uiOffset += 12;
			
				if (edit_vs_play == EditMode)
				{
					if (whichTrigPanel != SXTrigPanelNil)
					{
						int step = hexUIToStep(tNoteStack_first(&editStack));
					
						if (!getParameterFromStep(currentInstrument, step, On1 + whichPanel))
						{
							setParameterForEditStackSteps(currentInstrument, On1 + whichPanel, 1);
						
							manta_set_LED_hex(whichPanel + uiOffset, Red);
						}
						else
						{
							setParameterForEditStackSteps(currentInstrument, On1 + whichPanel, 0);
						
							manta_set_LED_hex(whichPanel + uiOffset, Amber);
						}
					}
				
				
				}
				else if (edit_vs_play == TrigToggleMode)
				{
				
					int prevPanel = currentPanel[currentInstrument];
					currentPanel[currentInstrument] = whichPanel;
				
					setTriggerPanelLEDsFor(currentInstrument, currentPanel[currentInstrument]);
					
					allUIStepsOff((currentInstrument+1)%2);
				
					manta_set_LED_hex(currentUpperHexUI, Red);
				}
				else //PlayToggleMode
				{
				
				}
			}
			
		}
			
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
		setSliderLEDsFor(currentInstrument, hexUIToStep(current_pattern_hex));
	}
	
	if (glideNoteOn >= 0)
	{
		glideNoteOn = -1;
		currentMantaSliderMode = prevMantaSliderMode;
		setSliderLEDsFor(currentInstrument, hexUIToStep(current_pattern_hex));
	}
}

void setSequencerLEDs(void)
{
	if (full_vs_split == FullMode)
	{
		setSequencerLEDsFor(currentInstrument);
	}
	else //SplitMode
	{
		setSequencerLEDsFor(InstrumentOne);
		setSequencerLEDsFor(InstrumentTwo);
	}
}

void setKeyboardLEDs(void)
{
	for (int i = 0; i < 48; i++)
	{
		manta_set_LED_hex(i, Off);
	}
}

// ~ ~ ~ ~ TOP LEFT BUTTON ~ ~ ~ ~ //
void touchTopLeftButton(void)
{
	if (key_vs_option == OptionMode) return;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	
	//toggle three different SliderModes in PitchMode
	if (sequencer->pitchOrTrigger == PitchMode)
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
			setSliderLEDsFor(currentInstrument, -1);
		}
		else
		{
			setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
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
		setSequencerLEDsFor(InstrumentOne);
		setSequencerLEDsFor(InstrumentTwo);
	}
	else
	{
		setSequencerLEDsFor(currentInstrument);
	}
	
	if (edit_vs_play == PlayToggleMode)
	{
		switchToMode(EditMode);
	}
	else // EditMode
	{
		switchToMode(PlayToggleMode);
	}
	
	setSliderLEDsFor(currentInstrument, hexUIToStep(tNoteStack_first(&editStack)));
}

void releaseTopRightButton(void)
{
	compositionAction = CompositionRead;
	copyStage = 0;
	copyWhichInst = InstrumentNil;
	copyWhichComp = -1;
	
	if (key_vs_option == OptionMode) setCompositionLEDs();
	else                             setSequencerLEDs();
}

// ~ ~ ~ ~ BOTTOM LEFT BUTTON ~ ~ ~ ~ //
void touchBottomLeftButton(void)
{
	key_vs_option = OptionMode;
	
	allUIStepsOff(InstrumentOne);
	allUIStepsOff(InstrumentTwo);
	
	setOptionLEDs();

	manta_set_LED_button(ButtonBottomLeft, Red);
}


void releaseBottomLeftButton(void)
{	
	key_vs_option = KeyboardMode;
	
	if (edit_vs_play == TrigToggleMode)		setKeyboardLEDsFor(currentInstrument, 0);
	else /* PlayToggleMode or EditMode */	setKeyboardLEDsFor(currentInstrument, -1);
	
	if		(manta[currentInstrument].type == SequencerInstrument) setSequencerLEDs();
	else if (manta[currentInstrument].type == KeyboardInstrument) setKeyboardLEDs();
	else									;

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
	if (manta[currentInstrument].type != SequencerInstrument) return;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	
	int note = hexUIToStep(currentHexUI);
	
	int currStep = sequencer->currentStep;
	
	int uiHexCurrentStep = stepToHexUI(currentInstrument, currStep);

	if (currentMantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)
		{
			setParameterForEditStackSteps(currentInstrument, CV1, val);
			//sequencer[currentSequencer].step[note].cv1 = val;
		}
		else // SliderTwo
		{
			setParameterForEditStackSteps(currentInstrument, CV2, val << 4);
		}
		
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off

		if (tNoteStack_contains(&editStack, uiHexCurrentStep) != -1)
		{
			if (sliderNum == SliderOne)
			{
				dacsend(currentInstrument * 2, 1, val);
			}
			else
			{
				testval = val << 5;
				DAC16Send(2*currentInstrument+1, val << 4);
			}
			
		}
	}
	else if (currentMantaSliderMode == SliderModeTwo)
	{
		// check if you're in second slider mode, where top slider is cv3 out and bottom slider is cv4 out
		if (sliderNum == SliderOne)
		{
			setParameterForEditStackSteps(currentInstrument, CV3, val);
			//sequencer[currentSequencer].step[note].cv3 = val;
		}
		else // SliderTwo
		{
			setParameterForEditStackSteps(currentInstrument, CV4, val);
			//sequencer[currentSequencer].step[note].cv4 = val;
		}

		
		manta_set_LED_slider(sliderNum,(val >> 9) + 1); // add one to the slider values because a zero turns them off
		
		if (tNoteStack_contains(&editStack,uiHexCurrentStep) != -1)
		{
			if (sliderNum == SliderOne)
			{
				dacsend(currentInstrument * 2 + 1, 0, val);
			}
			else
			{
				dacsend(currentInstrument * 2 + 1, 1, val);
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
				prevOct = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentInstrument,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
			{
				DAC16Send(2 * currentInstrument, get16BitPitch(currentInstrument, currStep)); 
			}
		}
		else //SliderTwo
		{
			setParameterForEditStackSteps(currentInstrument, Length, (val >> 9) + 1);
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
				prevOct = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].octave;
			}
			uint16_t newOct = (val >> 9);
			
			setParameterForEditStackSteps(currentInstrument,Octave,newOct);
			
			manta_set_LED_slider(SliderOne, newOct + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevOct != newOct))
			{
				DAC16Send(2 * currentInstrument, get16BitPitch(currentInstrument, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
		}
		else // SliderTwo
		{
			// Fine Tune
			uint16_t prevFine = 0;
			if (editStack.size <= 1)
			{
				prevFine = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].fine;
			}
			uint16_t newFine = val;
			
			setParameterForEditStackSteps(currentInstrument, Fine, val);
			
			manta_set_LED_slider(SliderTwo, (val >> 9) + 1); // add one to the slider values because a zero turns them off

			if ((tNoteStack_contains(&editStack,uiHexCurrentStep) != -1) && (prevFine != newFine))
			{
				DAC16Send(2 * currentInstrument, get16BitPitch(currentInstrument, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
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
				if (sequencer->pitchOrTrigger == PitchMode) 
					prevGlide = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].pglide;
				else
					prevGlide = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
				
			}
			uint16_t newGlide = val;
			
			if (newGlide < 5) newGlide = 5;
			
			if (sequencer->pitchOrTrigger == PitchMode) 
				setParameterForEditStackSteps(currentInstrument, PitchGlide, newGlide);
			else
				setParameterForEditStackSteps(InstrumentTwo, CVGlide, newGlide);
				
			manta_set_LED_slider(SliderOne, (val >> 9)  + 1); // add one to the slider values because a zero turns them off
		}
		else // SliderTwo
		{
			// CV glide time
			uint16_t prevGlide = 0;
			if (editStack.size <= 1)
			{
				if (sequencer->pitchOrTrigger == PitchMode)
					prevGlide = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
				else
					prevGlide = sequencer->step[hexUIToStep(tNoteStack_first(&editStack))].cvglide;
			}
			uint16_t newGlide = val;
			
			if (newGlide < 5) newGlide = 5;
			
			if (sequencer->pitchOrTrigger == PitchMode)
				setParameterForEditStackSteps(currentInstrument, CVGlide, newGlide);
			else
				setParameterForEditStackSteps(InstrumentOne, CVGlide, newGlide);
			
			manta_set_LED_slider(SliderTwo, (val >> 9)  + 1); // add one to the slider values because a zero turns them off
		}
	}
	else
	{
		// Should not get here.
	}
}

void uiStep(MantaInstrument inst)
{
	if (key_vs_option == OptionMode) return;
	
	if (full_vs_split == FullMode && inst != currentInstrument) return;
	
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int cStep = sequencer->currentStep;
	int pStep = sequencer->prevStep;
	int uiHexCurrentStep = stepToHexUI(inst, cStep);
	int uiHexPrevStep = stepToHexUI(inst, pStep);

	if (edit_vs_play == EditMode)
	{
		if (sequencer->pitchOrTrigger == PitchMode)
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
				if (sequencer->step[cStep].on[currentPanel[currentInstrument]])
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
				if (sequencer->step[pStep].on[currentPanel[currentInstrument]])
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
		if (sequencer->pitchOrTrigger == TriggerMode)
		{
			if (sequencer->step[pStep].toggled)
			{
				manta_set_LED_hex(uiHexPrevStep, Amber);
			}

			if (!sequencer->step[cStep].on[currentPanel[currentInstrument]])
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
			if (sequencer->notestack.size > 0) manta_set_LED_hex(uiHexCurrentStep, RedOn);
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
	manta_set_LED_hex(currentPanel[InstrumentOne] + MAX_STEPS, Red);
	manta_set_LED_hex(currentPanel[InstrumentTwo] + MAX_STEPS + 12, Red);	
}

void setKeyboardLEDsFor(MantaInstrument inst, int note)
{	
	tSequencer* sequencer = &manta[inst].sequencer;
	int setRed = 1;
	int nt = 0;
	
	if (note < 0)	setRed = 0;
	else			nt = note;
	
	if (sequencer->pitchOrTrigger == PitchMode)
	{
		if (sequencer->step[nt].note == 1)
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
				manta_set_LED_hex(sequencer->step[nt].kbdhex, Red);
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
		
		if (inst == InstrumentTwo)
		{
			hexUIOffset += 12;
		}
		
		if (setRed)
		{
			for (int panel = 0; panel < 4; panel++)
			{
				manta_set_LED_hex(hexUIOffset + panel, (sequencer->step[note].on[panel]) ? Red : Amber);
			}
		}		
	}
	
	manta_set_LED_button(ButtonTopRight, (edit_vs_play == EditMode) ? Red : Amber);
	manta_set_LED_button(ButtonBottomLeft, Off);
	manta_set_LED_button(ButtonBottomRight, (playSubMode == SeqMode) ? Off : Amber);
	manta_set_LED_button(ButtonTopLeft, (currentMantaSliderMode == SliderModeOne) ? Off : (currentMantaSliderMode == SliderModeTwo) ? Amber : Red);
}


void setSliderLEDsFor(MantaInstrument inst, int note)
{
	if (manta[inst].type == SequencerInstrument)
	{
		tSequencer* sequencer = &manta[inst].sequencer;
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
			cv1 = (sequencer->step[note].cv1 >> 9) + 1;
			cv2 = (sequencer->step[note].cv2 >> 9) + 1;
			cv3 = (sequencer->step[note].cv3 >> 9) + 1;
			cv4 = (sequencer->step[note].cv4 >> 9) + 1;
			fine = (sequencer->step[note].fine >> 9) + 1;
			octave = (sequencer->step[note].octave + 1);
			length = (sequencer->step[note].length);
			pglide = (sequencer->step[note].pglide >> 9) + 1;
			cvglide = (sequencer->step[note].cvglide >> 9) + 1;
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
			if (sequencer->pitchOrTrigger == PitchMode)
			{
				manta_set_LED_slider(SliderOne, pglide); // OCTAVE add one to the slider values because a zero turns them off
				manta_set_LED_slider(SliderTwo, cvglide); // the step length is already between 1-8
			}
			else // TriggerMode
			{
				manta_set_LED_slider((inst == InstrumentOne) ? SliderOne : SliderTwo, (sequencer->step[note].cvglide >> 9) + 1);
				
			}
			
		}
		
	}
	
	
	
}

void setTriggerPanelLEDsFor(MantaInstrument inst, TriggerPanel panel)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int hexUI = 0;
	for (int i = 0; i < sequencer->maxLength; i++)
	{
		hexUI = stepToHexUI(inst,i);
		
		manta_set_LED_hex(hexUI,Off);
		
		if (sequencer->step[i].on[panel])
		{
			manta_set_LED_hex(hexUI,Red);
		}
		else
		{
			manta_set_LED_hex(hexUI,Off);
		}
	}
}

void setSequencerLEDsFor(MantaInstrument inst)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int hexUI = 0;

	for (int i = 0; i < sequencer->maxLength; i++)
	{
		hexUI = stepToHexUI(inst, i);

		manta_set_LED_hex(hexUI, (sequencer->step[i].toggled) ? Amber : Off);
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
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	
	MantaInstrumentType type = manta[currentInstrument].type;

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
			if (type == SequencerInstrument && sequencer->pitchOrTrigger == PitchMode)	manta_set_LED_hex(hex, Red);
			else																		manta_set_LED_hex(hex, Amber);
			
		}
		else if (option == OptionTrigger)
		{
			if (type == SequencerInstrument && sequencer->pitchOrTrigger == TriggerMode) manta_set_LED_hex(hex, Red);
			else																		 manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionKeyboard)
		{
			if (type == KeyboardInstrument)	manta_set_LED_hex(hex, Red);
			else							manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionFullSplit)
		{
			manta_set_LED_hex(hex, (full_vs_split == FullMode) ? Amber : Red);
		}
		else if (option <= OptionPatternEight)
		{
			manta_set_LED_hex(hex, (option == sequencer->pattern) ? Red : Amber);
		}
		else if (option == OptionLeft)
		{
			manta_set_LED_hex(hex, (currentInstrument == InstrumentOne) ? Red : Amber);
		}
		else if (option == OptionRight)
		{
			manta_set_LED_hex(hex, (currentInstrument == InstrumentTwo) ? Red : Amber);
		}
	}

	
	if (manta[currentInstrument].type == SequencerInstrument)   
	{
		manta_set_LED_button(ButtonBottomRight, Red); // Write
		manta_set_LED_button(ButtonTopRight, Amber); // Copy
		manta_set_LED_button(ButtonTopLeft, Off); // Nothing for now
		
		setCompositionLEDs();
	}
	else if (manta[currentInstrument].type == KeyboardInstrument)
	{    
		setKeyboardConfigLEDs();
	}
	else // DirectInstrument
	{
		
	}

}

// Output
uint32_t get16BitPitch(MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	// take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
	uint32_t DACtemp = ((uint32_t)sequencer->step[step].pitch);
	DACtemp += (sequencer->step[step].octave * 12);
	DACtemp *= 546125;
	DACtemp /= 1000;

	DACtemp += (sequencer->step[step].fine >> 2) - 512;
	
	return DACtemp;
}

uint16_t glideTimeTEMP = 0;
float destTEMP= .0f;

void dacSendPitchMode(MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	if (sequencer->step[step].note)
	{
		// Configure PitchGlide
		tRamp* glide = &out[inst][CVPITCH];
		
		uint16_t glideTime =  sequencer->step[step].pglide >> 3;
		if (glideTime < 5) glideTime = 5;
		
		tRampSetDest(glide, (float)get16BitPitch(inst,step) / UINT16_MAX); 
		tRampSetTime(glide, glideTime);

		// Configure CVGlide
		glideTime =  sequencer->step[step].cvglide >> 3;
		if (glideTime < 5) glideTime = 5;
		

		tRampSetTime(&out[inst][CV1P], glideTime);
		tRampSetDest(&out[inst][CV1P], (float) sequencer->step[step].cv1);
		
		tRampSetTime(&out[inst][CV2P], glideTime);
		tRampSetDest(&out[inst][CV2P], (float) sequencer->step[step].cv2);
		
		tRampSetTime(&out[inst][CV3P], glideTime);
		tRampSetDest(&out[inst][CV3P], (float) sequencer->step[step].cv3);
		
		tRampSetTime(&out[inst][CV4P], glideTime);
		tRampSetDest(&out[inst][CV4P], (float) sequencer->step[step].cv4);
		
		// Send Trigger
		dacsend(2*inst,0,4095);
	}
}


void dacSendTriggerMode(MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	// Configure CVGlide
	uint16_t glideTime =  sequencer->step[step].cvglide >> 3;
	if (glideTime < 5) glideTime = 5;
	

	tRampSetTime(&out[inst][CV1T], glideTime);
	tRampSetDest(&out[inst][CV1T], (float) sequencer[inst].step[step].cv1);
		
	tRampSetTime(&out[inst][CV2T], glideTime);
	tRampSetDest(&out[inst][CV2T], (float) sequencer[inst].step[step].cv2);

	
	// Trigger 1, Trigger 2, Trigger 3, Trigger 4
	dacsend(2*inst+0, 0, sequencer[inst].step[step].on[0] * 4095);
	dacsend(2*inst+1, 0, sequencer[inst].step[step].on[1] * 4095);
	dacsend(2*inst+0, 1, sequencer[inst].step[step].on[2] * 4095);
	dacsend(2*inst+1, 1, sequencer[inst].step[step].on[3] * 4095);
}

// UTILITIES
uint16_t getParameterFromStep(MantaInstrument inst, uint8_t step, StepParameterType param)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	if (param == Toggled)		return sequencer->step[step].toggled;
	else if (param == Length)	return sequencer->step[step].length;
	else if (param == CV1)		return sequencer->step[step].cv1;
	else if (param == CV2)		return sequencer->step[step].cv2;
	else if (param == CV3)		return sequencer->step[step].cv3;
	else if (param == CV4)		return sequencer->step[step].cv4;
	else if (param == Pitch)	return sequencer->step[step].pitch;
	else if (param == Fine)		return sequencer->step[step].fine;
	else if (param == Octave)	return sequencer->step[step].octave;
	else if (param == Note)		return sequencer->step[step].note;
	else if (param == KbdHex)	return sequencer->step[step].kbdhex;
	else if (param == PitchGlide)	return sequencer->step[step].pglide;
	else if (param == CVGlide)	return sequencer->step[step].cvglide;
	else if (param == On1)		return sequencer->step[step].on[PanelOne];
	else if (param == On2)		return sequencer->step[step].on[PanelTwo];
	else if (param == On3)		return sequencer->step[step].on[PanelThree];
	else if (param == On4)		return sequencer->step[step].on[PanelFour];
	else return 0;
}

void setParameterForStep(MantaInstrument inst, uint8_t step, StepParameterType param, uint16_t value)
{
	
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)			sequencer->step[step].toggled = value;
	else if (param == Length)		sequencer->step[step].length = value;
	else if (param == CV1)			sequencer->step[step].cv1 = value;
	else if (param == CV2)			sequencer->step[step].cv2 = value;
	else if (param == CV3)			sequencer->step[step].cv3 = value;
	else if (param == CV4)			sequencer->step[step].cv4 = value;
	else if (param == Pitch)		sequencer->step[step].pitch = value;
	else if (param == Fine)			sequencer->step[step].fine = value;
	else if (param == Octave)		sequencer->step[step].octave = value;
	else if (param == Note)			sequencer->step[step].note = value;
	else if (param == KbdHex)		sequencer->step[step].kbdhex = value;
	else if (param == PitchGlide)	sequencer->step[step].pglide = value;
	else if (param == CVGlide)		sequencer->step[step].cvglide = value;
	else if (param == On1)			sequencer->step[step].on[PanelOne] = value;
	else if (param == On2)			sequencer->step[step].on[PanelTwo] = value;
	else if (param == On3)			sequencer->step[step].on[PanelThree] = value;
	else if (param == On4)			sequencer->step[step].on[PanelFour] = value;
	else;
}

void setParameterForEditStackSteps(MantaInstrument inst, StepParameterType param, uint16_t value)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].toggled = value;
	else if (param == Length)		for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].length = value;
	else if (param == CV1)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].cv1 = value;
	else if (param == CV2)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].cv2 = value;
	else if (param == CV3)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].cv3 = value;
	else if (param == CV4)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].cv4 = value;
	else if (param == Pitch)		for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].pitch = value;
	else if (param == Fine)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].fine = value;
	else if (param == Octave)		for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].octave = value;
	else if (param == Note)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].note = value;
	else if (param == KbdHex)		for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].kbdhex = value;
	else if (param == PitchGlide)   for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].pglide = value;
	else if (param == CVGlide)		for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].cvglide = value;
	else if (param == On1)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelOne] = value;
	else if (param == On2)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelTwo] = value;
	else if (param == On3)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelThree] = value;
	else if (param == On4)			for (; i < size; i++) sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelFour] = value;
	else;
}

void upOctaveForEditStackSteps(MantaInstrument inst) 
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	for (int i = 0; i < editStack.size; i++) 
	{
		tStep* theStep = &(sequencer->step[hexUIToStep(editStack.notestack[i])]);
		
		if (theStep->octave < 7)	theStep->octave += 1;
	}
}

void downOctaveForEditStackSteps(MantaInstrument inst) 
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	for (int i = 0; i < editStack.size; i++)
	{
		tStep* theStep = &(sequencer->step[hexUIToStep(editStack.notestack[i])]);

		if (theStep->octave < 7)	theStep->octave -= 1;
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

uint8_t stepToHexUI(MantaInstrument inst, uint8_t stepIn)
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
		
		if (inst == InstrumentOne)
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