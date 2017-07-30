/*
 * sequencer_process.c
 *
 * Created: 6/10/2016 1:44:39 PM
 *  Author: Jeff Snyder
 */ 
#include "asf.h"
#include "main.h"
#include "manta_keys.h"
#include "uhi_hid_manta.h"
#include "utilities.h"
//#include "7Segment.h"


SubShift shiftOption1SubShift = SubShiftNil;
SubShift shiftOption2SubShift = SubShiftNil;

int copyStage = 0;
MantaInstrument copyWhichInst = InstrumentOne;
int copyWhichComp = -1;

PanelSwitch panelSwitch[NUM_PANEL_MOVES] =
{
	PanelLeft,
	PanelRight
};




// UTILITIES
void seqwait(void);
uint32_t get16BitPitch(tTuningTable* myTable, MantaInstrument, uint8_t step);
void setCurrentInstrument(MantaInstrument);
void setParameterForEditStackSteps(MantaInstrument, StepParameterType param, uint16_t value);
void setParameterForStep(MantaInstrument, uint8_t step, StepParameterType param, uint16_t value);
uint16_t getParameterFromStep(MantaInstrument, uint8_t step, StepParameterType param);
void resetEditStack(void);
uint8_t hexUIToStep(uint8_t hexagon);
uint8_t stepToHexUI(MantaInstrument, int step);
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

typedef enum OptionType
{
	OptionPatternOne = 0,
	OptionPatternTwo,
	OptionPatternThree,
	OptionPatternFour,
	OptionPatternFive,
	OptionPatternSix,
	OptionPatternSeven,
	OptionPatternEight,
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
	OptionPitch,
	OptionTrigger,
	OptionFullSplit,
	OptionToggle,
	OptionArp,
	OptionTouch,
	OptionKeyArp,
	OptionMono,
	OptionDuo,
	OptionTrio,
	OptionQuad,
	OptionSixOut,
	OptionTwelveOut,
	OptionLeft,
	OptionRight,
	OptionEditType,
	OptionNil
	
}OptionType;

typedef enum OptionModeType
{
	SequencerOptionMode,
	KeyboardOptionMode,
	DirectOptionMode,
	RightOptionMode,
	OptionModeNil
	
} OptionModeType;

OptionModeType currentOptionMode = OptionModeNil;

OptionType sequencerOptionMode[16] =	{	
	OptionPitch,
	OptionTrigger,
	OptionNil,
	OptionToggle,
	OptionArp, 
	OptionTouch,
	OptionNil, 
	OptionEditType,	
	
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
	OptionNil, 
	OptionFullSplit,
	OptionNil,
	OptionLeft, 
	OptionRight
};

OptionType keyboardOptionMode[16] =	{
	OptionMono,
	OptionDuo,
	OptionTrio,
	OptionQuad,
	OptionNil,
	OptionKeyArp,
	OptionNil,
	OptionNil,
	
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
	OptionNil,
	OptionFullSplit,
	OptionNil,
	OptionLeft,
	OptionRight
};

OptionType rightOptionMode[16] = {
	OptionPatternOne,
	OptionPatternTwo,
	OptionPatternThree,
	OptionPatternFour,
	OptionPatternFive,
	OptionPatternSix,
	OptionPatternSeven,
	OptionPatternEight,
	
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
	OptionNil,
	OptionFullSplit,
	OptionNil,
	OptionLeft,
	OptionRight
};





OptionType directOptionMode[16] = {
	OptionSixOut,
	OptionTwelveOut,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionLeft,
	OptionRight
	
};

OptionType defaultOptionMode[16] = {
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	OptionNil,
	
	OptionSequencer,
	OptionKeyboard,
	OptionDirect,
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
tNoteStack noteOnStack; // all notes on at any point during runtime


uint8_t encodeBuffer[NUM_INST][NUM_BYTES_PER_SEQUENCER]; 
uint8_t decodeBuffer[NUM_INST][NUM_BYTES_PER_SEQUENCER];
uint8_t memoryInternalCompositionBuffer[NUM_INST][NUM_BYTES_PER_COMPOSITION_BANK_ROUNDED_UP]; //8610 is 615 (number of bytes per sequence) * 14 (number of sequences that can be stored for each sequencer channel)

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
		currentOptionMode = (shiftOption2) ? RightOptionMode :
		(manta[inst].type == SequencerInstrument) ? SequencerOptionMode :
		(manta[inst].type == KeyboardInstrument) ? KeyboardOptionMode :
		(manta[inst].type == DirectInstrument) ? DirectOptionMode :
		OptionModeNil;
	}
	
}

bool blinkToggle;
void blink(void)
{
	blinkToggle = (blinkToggle == true) ? false : true;		   
	
	if (shiftOption1 && shiftOption1SubShift == SubShiftTopLeft)
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

void initMantaSequencer(void)
{
	takeover = FALSE;
	
	// Sequencer Modes
	currentMantaSliderMode =	SliderModeOne;
	prevMantaSliderMode =		SliderModeOne;
	edit_vs_play =				PlayToggleMode; manta_set_LED_button(ButtonTopRight, Amber);
	currentFunctionButton =		ButtonTopLeft;
	shiftOption1 =				FALSE;
	shiftOption2 =				FALSE;
	shiftOption1Lock =			FALSE;
	shiftOption2Lock =			FALSE;
	
	
	currentHexUI = -1;
	resetEditStack();
	
	// Initialize the noteOnStack. :D !!!
	tNoteStack_init(&noteOnStack, 32); 
	// Glad I commented that - thanks @Josh Becker for the tip on appropriate commenting.
	
	setCurrentInstrument(InstrumentOne);
	keyNoteOn = -1;
	glideNoteOn = -1;
	
	/////WE ARE CHANGING THIS AND THE WHOLE WAY IT WORKS
	if (preset_num == 0)
	{
		for (int i = 0; i < NUM_INST; i++)
		{
			manta[i].type = SequencerInstrument;
			if (preset_num == 0)
			{
				//if preset_num == 0, then we are trying to initialize the blank default sequencer
				manta[i].type = SequencerInstrument;
				tSequencer_init(&manta[i].sequencer, PitchMode, 32);
				tIRampSetTime(&out[i][CVTRIGGER], 0);

				//tSequencer_encode(&manta[i].sequencer, encodeBuffer[i]);
				//memoryInternalWriteSequencer(i, 0, encodeBuffer[i]);
				
				compositionMap[i][0] = false;
				currentComp[i] = 0;
			}
			else
			{   //otherwise we are loading a saved preset, so we need to prepare that data properly
				tSequencer_init(&manta[i].sequencer, PitchMode, 32);
				//tSequencer_encode(&manta[i].sequencer, encodeBuffer[i]);
				//memoryInternalWriteSequencer(i, 0, encodeBuffer[i]);
				compositionMap[i][0] = false;
				currentComp[i] = 0;
				//tSequencer_decode(&manta[i].sequencer, decodeBuffer[i]);
			}
		}
	}
	
	setSequencerLEDsFor(currentInstrument);
	
	setKeyboardLEDsFor(currentInstrument, -1);
}



void keyboardStep(MantaInstrument inst)
{
	 tKeyboard* keyboard = (inst == InstrumentNil) ? &fullKeyboard : &manta[inst].keyboard;
	 
	 if (keyboard->playMode != ArpMode) return;
	 
	 tKeyboard_nextNote(keyboard);
	 
	 if (++keyboard->currentVoice == keyboard->numVoices) keyboard->currentVoice = 0;
	 
	 dacSendKeyboard(inst);
}


void sequencerStep(MantaInstrument inst)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	if (takeover || (sequencer->playMode == TouchMode)) return;
	
	sequencer->lengthCounter += 1;
		
	if (sequencer->lengthCounter >= sequencer->step[sequencer->currentStep].length)
	{
		tSequencer_next(sequencer); // Move to next step, seq 1.
			
		if (sequencer->stepGo)
		{
			if (sequencer->pitchOrTrigger == PitchMode)
			{
				dacSendPitchMode(inst, sequencer->currentStep);
			}
			else // TriggerMode
			{
				dacSendTriggerMode(inst, sequencer->currentStep);
			}
		}
		sequencer->lengthCounter = 0;
	}
	
	uiStep(inst);

}

void jumpToStep(MantaInstrument inst, int step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	sequencer->currentStep = step;
	
	if (sequencer->pitchOrTrigger == PitchMode)
	{
		dacSendPitchMode(inst, step);
	}
	else // TriggerMode
	{
		dacSendTriggerMode(inst, step);
	}


}

MantaButton lastFunctionButton;


void releaseDirectHex(int hex)
{
	DirectType type;
	if (!takeover)
	{
		tDirect* direct = &manta[currentInstrument].direct;
		
		type = direct->hexes[hex].type;
		
		int output = tDirect_getOutput(direct, hex);
		
		if (type == DirectGate)
		{
			sendDataToOutput(6*currentInstrument + output, 0, 0x0);
		}
	}
	else
	{
		tDirect* direct = &fullDirect;
			
		type = direct->hexes[hex].type;
		
		int output = tDirect_getOutput(direct, hex);
			
		if (type == DirectGate)
		{
			sendDataToOutput(output, 0, 0x0);
		}
	}
	
	manta_set_LED_hex(hex,	(type == DirectCV) ? Amber :
							(type == DirectTrigger) ? BothOn :
							(type == DirectGate) ? Red : Off);
	
	
}

void touchDirectHex(int hex)
{
	if (!takeover)
	{
		tDirect* direct = &manta[currentInstrument].direct;
		
		DirectType type = direct->hexes[hex].type;
		int output = direct->hexes[hex].output;
		
		if (type == DirectGate)
		{
			sendDataToOutput(6*currentInstrument + output, 0, 0xffff);
		}
		else if (type == DirectTrigger)
		{
			// set output high then start timer
			direct->hexes[hex].trigCount = TRIGGER_TIMING;

			sendDataToOutput(6*currentInstrument + output, 0, 0xffff);
		}
	}
	else
	{
		tDirect* direct = &fullDirect;
		
		DirectType type = direct->hexes[hex].type;
		int output = direct->hexes[hex].output;
		
		if (type == DirectGate)
		{
			sendDataToOutput(output, 0, 0xffff);
		}
		else if (type == DirectTrigger)
		{
			// set output high then start timer
			direct->hexes[hex].trigCount = TRIGGER_TIMING;
			
			sendDataToOutput(output, 0, 0xffff);
		}
	}
	
	
	if (!directEditMode) manta_set_LED_hex(hex, Off);
	
}

void processHexTouch(void)
{
	uint8_t newHexUIOn = 0;
	uint8_t newHexUIOff = 0;
	uint8_t newUpperHexUI = 0;
	MantaInstrumentType type = takeover ? takeoverType : manta[currentInstrument].type;

	//check the sequencer step hexagons
	for (int i = 0; i < 48; i++)
	{
		// RELEASE
		if ((butt_states[i] <= 0) && (pastbutt_states[i] > 0))
		{
			if (hexmapEditMode)
			{
				releaseHexmapEdit(i);
			}
			else if (directEditMode)
			{
				releaseDirectEdit(i);
				releaseDirectHex(i);
			}
			else if (shiftOption1 || shiftOption2) 
			{
				if (i < MAX_STEPS)	releaseLowerHexOptionMode(i);
				else				releaseUpperHexOptionMode(i);
				
				releaseLingeringKeyboardHex(i);
				// may need releaseLingeringDirectHex(i);
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
					
					releaseLingeringKeyboardHex(i);
				}
				else if (type == KeyboardInstrument)
				{
					releaseKeyboardHex(i);
				}
				else if (type == DirectInstrument)
				{
					releaseDirectHex(i);
				}
			}
			
		}
		
		// TOUCH
		if ((butt_states[i] > 0) && (pastbutt_states[i] <= 0)) // TOUCH
		{
			if (hexmapEditMode)
			{
				touchHexmapEdit(i, butt_states[i]);
			}
			else if (directEditMode)
			{
				touchDirectEdit(i);
				touchDirectHex(i);
			}
			else if (shiftOption1 || shiftOption2) 
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
				else if (type == KeyboardInstrument)
				{
					touchKeyboardHex(i, butt_states[i]);
				}
				else if (type == DirectInstrument)
				{
					touchDirectHex(i);
				}
			}
		}
		
		pastbutt_states[i] = butt_states[i];
	}
	
	BOOL buttonTouched = FALSE;
	BOOL topRon = FALSE;
	BOOL topLon = FALSE;
	for (int i = 0; i < 4; i++)
	{
		if ((func_button_states[i] > 0) && (past_func_button_states[i] <= 0))
		{
			//a round function button was just pressed
			resetSliderMode();
			
			buttonTouched = TRUE;
			
			if (i == ButtonBottomLeft)				touchBottomLeftButton();
			else if (directEditMode)
			{
				if (i == ButtonTopLeft)
				{
					topLon = TRUE;
					touchDirectEdit(48); // SliderOne
				}
				else if (i == ButtonTopRight)
				{
					topRon = TRUE;
					touchDirectEdit(49); // SliderTwo
				}
			}
			else if (!(hexmapEditMode || directEditMode))
			{
				if (i == ButtonTopLeft)
				{
					topLon = TRUE;
					touchTopLeftButton();
				}
				else if (i == ButtonTopRight)			
				{
					topRon = TRUE;
					touchTopRightButton();
				}
				else if (i == ButtonBottomRight)	touchBottomRightButton();
			}	
			
		}
		
		if ((func_button_states[i] <= 0) && (past_func_button_states[i] > 0))
		{		
			buttonTouched = TRUE;
			
			if (directEditMode)
			{
				if (i == ButtonTopLeft)
				{
					releaseDirectEdit(48); // SliderOne
				}
				else if (i == ButtonTopRight)
				{
					releaseDirectEdit(49); // SliderTwo
				}
			}
			else if (!(hexmapEditMode || directEditMode))
			{
				if (i == ButtonBottomLeft)			releaseBottomLeftButton();
				else if (i == ButtonTopRight)		releaseTopRightButton();
				else if (i == ButtonTopLeft)		releaseTopLeftButton();
				else if (i == ButtonBottomRight)	releaseBottomRightButton();
			}
		}
		
		if (buttonTouched && !(hexmapEditMode || directEditMode))
		{
			if (manta[currentInstrument].type == SequencerInstrument)
			{
				manta_set_LED_button(ButtonTopLeft, (shiftOption1 || shiftOption2) ? (topLon ? Amber : Off) :
				(currentMantaSliderMode == SliderModeOne) ? Off :
				(currentMantaSliderMode == SliderModeTwo) ? Amber : Red);
				manta_set_LED_button(ButtonTopRight, (shiftOption1 || shiftOption2) ? (topRon ? Amber : Off) :
				(edit_vs_play == EditMode) ? Red : Amber);
				manta_set_LED_button(ButtonBottomLeft, (shiftOption1 ? Amber : (shiftOption2Lock ? Red : Off)));
				manta_set_LED_button(ButtonBottomRight, (shiftOption2 ? Amber : (shiftOption1Lock ? Red : Off)));
			}
			else if (manta[currentInstrument].type == KeyboardInstrument)
			{
				manta_set_LED_button(ButtonTopLeft, (topLon  ? ((shiftOption1 || shiftOption2) ? Amber : Red) : Off) );
				manta_set_LED_button(ButtonTopRight, (topRon  ? ((shiftOption1 || shiftOption2) ? Amber : Red) : Off));
				manta_set_LED_button(ButtonBottomLeft, (shiftOption1 ? Amber : (shiftOption2Lock ? Red : Off)));
				manta_set_LED_button(ButtonBottomRight, (shiftOption2 ? Amber : (shiftOption1Lock ? Red : Off)));
			}
			else if (manta[currentInstrument].type == DirectInstrument)
			{
				manta_set_LED_button(ButtonBottomLeft, (shiftOption1 ? Amber : (shiftOption2Lock ? Red : Off)));
				manta_set_LED_button(ButtonBottomRight, (shiftOption2 ? Amber : (shiftOption1Lock ? Red : Off)));
			}
		}
		
		past_func_button_states[i] = func_button_states[i];
	}		
	
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
		
		tSequencer* sequencer = &manta[currentInstrument].sequencer;
		
		if (sequencer->playMode == ArpMode)
		{
			tSequencer_toggleStep(sequencer,step);
				
			manta_set_LED_hex(hexagon, Off);
		} 
		else // SeqMode or TouchMode, ignore
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

void setHexmapLEDs(void)
{
	
}

void setHexmapConfigureLEDs	(void)
{
	if (!takeover)
	{
		for (int inst = 0; inst < 2; inst++)
		{
			if (manta[inst].type == KeyboardInstrument)
			{
				for (int i = 0; i < 16; i++) manta_set_LED_hex(16*inst + i, Off);
				manta_set_LED_hex(16*inst + 8, (firstEdition ? Amber : Red)); // SAVE
				manta_set_LED_hex(16*inst + 9, (firstEdition ? Amber : Red)); // LOAD
				manta_set_LED_hex(16*inst + 11, (firstEdition ? Amber : Red)); // EDIT
				
				manta_set_LED_hex(16*inst + 0, Amber); // Default
				manta_set_LED_hex(16*inst + 1, Amber); // Piano
				manta_set_LED_hex(16*inst + 2, Amber); // Harmonic
				manta_set_LED_hex(16*inst + 3, Amber); // Wicki
				manta_set_LED_hex(16*inst + 4, Amber); // Isomorphic
				manta_set_LED_hex(16*inst + 5, Amber); // Free
				manta_set_LED_hex(16*inst + 7, (firstEdition ? Amber : Red)); // BLANK
			}
		}
	}
	else if (takeoverType == KeyboardInstrument)
	{
		for (int i = 0; i < 32; i++) manta_set_LED_hex(i, Off);
		
		manta_set_LED_hex(8, (firstEdition ? Amber : Red)); // SAVE
		manta_set_LED_hex(9, (firstEdition ? Amber : Red)); // LOAD
		manta_set_LED_hex(11, (firstEdition ? Amber : Red)); // EDIT
		
		manta_set_LED_hex(0, Amber); // Default
		manta_set_LED_hex(1, Amber); // Piano
		manta_set_LED_hex(2, Amber); // Harmonic
		manta_set_LED_hex(3, Amber); // Werck
		manta_set_LED_hex(4, Amber); // Isomorphic
		manta_set_LED_hex(5, Amber); // Free
		manta_set_LED_hex(7, (firstEdition ? Amber : Red)); // BLANK
	}
	
	
}

void setCompositionLEDs(void)
{
	freeze_LED_update = 1;

	if (!takeover)
	{
		for (int inst = 0 ; inst < 2; inst++)
		{
			if (manta[inst].type == SequencerInstrument)
			{
				for (int i = 0; i < 16; i++) manta_set_LED_hex(i+16*inst,Off);
				
				for (int comp = 0; comp < NUM_COMP; comp++)
				{
					if (compositionMap[inst][comp])
					{
						if (shiftOption1SubShift == SubShiftNil)				manta_set_LED_hex(inst * 16 + comp, (currentComp[inst] == comp) ? Red : Amber);
						else if (shiftOption1SubShift == SubShiftTopRight)	manta_set_LED_hex(inst * 16 + comp, Red);
					}
				}
				
				manta_set_LED_hex(13 + 16*inst, Amber); // Deviate
				manta_set_LED_hex(14 + 16*inst, Amber); // Random
				manta_set_LED_hex(15 + 16*inst, firstEdition ? Amber : Red); // Blank
			}
			
		}

	}
	
	roll_LEDs = 1;
	freeze_LED_update = 0;
}

void touchLowerHexOptionMode(uint8_t hexagon)
{
	if (shiftOption1)
	{
		MantaInstrument whichInst = (hexagon < 16) ? InstrumentOne : InstrumentTwo;
		MantaInstrumentType type = manta[whichInst].type;
		int whichHex = (hexagon < 16) ? hexagon : (hexagon - 16);
		
		if (type == DirectInstrument)
		{	
			// DIRECT EDIT MODE
			editDirect = takeover ? &fullDirect : &manta[whichInst].direct;
			editDirectInstrument = takeover ? InstrumentNil : whichInst;
			
			if (whichHex == 8)
			{
				// Save
				manta_set_LED_hex(hexagon, Amber);
				
				Write7Seg(currentDirectSelect);
				
				displayState = DirectSelect;
			}
			else if (whichHex == 9)
			{
				// Load
				manta_set_LED_hex(hexagon, Amber);
				
				Write7Seg(currentDirectSelect);
				
				displayState = DirectSelect;
			}
			else if (whichHex == 11)
			{
				// Edit
				directEditMode = TRUE;
				lastDirectEditHex = -1;
				displayState = UpDownSwitchBlock;
				
				setDirectLEDs();
			}
			else if (whichHex <= 4)
			{
				tDirect_setConfiguration(editDirect, whichHex);
				
				if (takeover)
				{
					for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
				}
				else
				{
					for (int i = 0; i < 6; i++) sendDataToOutput(i+whichInst*6, globalCVGlide, 0);
				}
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 7)
			{
				// Blank
				tDirect_blank(editDirect);
				
				manta_set_LED_hex(hexagon, Amber);
			}
		}
		else if (type == KeyboardInstrument)
		{
			// HEXMAP EDIT MODE
			hexmapEditKeyboard = takeover ? &fullKeyboard : &manta[whichInst].keyboard;
			hexmapEditInstrument = takeover ? InstrumentNil : whichInst;
			
			if (whichHex == 8)
			{
				// Save
				manta_set_LED_hex(hexagon, Amber);
				
				Write7Seg(currentHexmapSelect);
				
				displayState = HexmapSelect;
			}
			else if (whichHex == 9)
			{
				// Load
				manta_set_LED_hex(hexagon, Amber);
				
				Write7Seg(currentHexmapSelect);
				
				displayState = HexmapSelect;
			}
			else if (whichHex == 11)
			{
				// Edit 
				hexmapEditMode = TRUE;
				lastHexmapEditHex = -1;
				displayState = UpDownSwitchBlock;
				
				manta_set_LED_slider(SliderOne, 0);
				manta_set_LED_slider(SliderTwo, 0);
				setKeyboardLEDs();
			}
			else if (whichHex == 0)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, DefaultMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 1)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, PianoMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 2)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, HarmonicMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 3)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, WickiHaydenMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 4)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, IsomorphicMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 5)
			{
				tKeyboard_setToDefault(hexmapEditKeyboard, FreeMap);
				
				manta_set_LED_hex(hexagon, Red);
			}
			else if (whichHex == 7)
			{
				// Blank
				tKeyboard_blankHexmap(hexmapEditKeyboard);
				
				manta_set_LED_hex(hexagon, Amber);
			}
			
		}
		else if (type == SequencerInstrument)
		{
			tSequencer* sequencer = &manta[whichInst].sequencer;
			
			if (whichHex == 13)
			{
				manta_set_LED_hex(whichInst * 16 + 13, Red);
			}
			else if (whichHex == 14)
			{
				manta_set_LED_hex(whichInst * 16 + 14, Red);
			}
			else if (whichHex == 15)
			{
				tSequencer_clear(sequencer); // EFFECTIVELY BLANK
				
				manta_set_LED_hex(whichInst * 16 + 15, Amber);
			}
			else
			{
				if (shiftOption1SubShift == SubShiftNil)
				{
					displayState = SequencerSelect;
					
					currentSequencerSelect = -1;
					Write7Seg(currentSequencerSelect);
					
					if (compositionMap[whichInst][whichHex])
					{
						memoryInternalReadSequencer(whichInst, whichHex,decodeBuffer[whichInst]);
						tSequencer_decode(sequencer, decodeBuffer[whichInst]);
						
						currentComp[whichInst] = whichHex;
						
						setCurrentInstrument(whichInst);
					}
					

				}
				else if (shiftOption1SubShift == SubShiftTopRight) // CompositionWrite
				{
					manta_set_LED_hex(hexagon, Amber);
					
					compositionMap[whichInst][whichHex] = true;
					tSequencer_encode(sequencer, encodeBuffer[whichInst]);
					memoryInternalWriteSequencer(whichInst, whichHex,encodeBuffer[whichInst]);
					
					setCompositionLEDs();
				}
				else if (shiftOption1SubShift == SubShiftTopLeft) //CompositionCopy
				{
					if (copyStage == 0)
					{
						// this determines sequence to be copied
						if (compositionMap[whichInst][whichHex])
						{
							copyWhichInst = whichInst;
							copyWhichComp = whichHex;
							
							copyStage = 1;
						}
					}
					else if (copyStage == 1)
					{
						// any hex pressed while still in copy mode is written with sequencer from copy stage 1
						if (!(whichInst == copyWhichInst && whichHex == copyWhichComp)) // if not the same step, copy
						{
							manta_set_LED_hex(hexagon, Red);
							compositionMap[whichInst][whichHex] = true;
							memoryInternalCopySequencer(copyWhichInst, copyWhichComp, whichInst, whichHex);
						}
					}
				}
			}
		}
	}
    else if (shiftOption2)
    {
		if (hexagon == 0) // load global tuning
		{
			if (currentTuningHex > 0) manta_set_LED_hex(currentTuningHex, Off);
			currentTuningHex = -1;
			manta_set_LED_hex(0, Amber);
			loadTuning(globalTuning);
		}
		else if (hexagon < 32)
		{
			 // Tuning hex
		    manta_set_LED_hex(currentTuningHex, Off);
			
			currentTuningHex = hexagon;
			
			manta_set_LED_hex(currentTuningHex, Amber);
			
			currentMantaUITuning = mantaUITunings[currentTuningHex];
			
			Write7Seg(currentMantaUITuning); // Set display
			
			loadTuning(currentMantaUITuning);
			
			if (shiftOption2Lock)
			{
				 displayState = TuningHexSelect;
			}
			
        }

    }
}

void releaseLowerHexOptionMode(uint8_t hexagon)
{
    if (shiftOption1)
    {
		MantaInstrument whichInst = takeover ? InstrumentNil : ((hexagon < 16) ? InstrumentOne : InstrumentTwo);
		MantaInstrumentType type = takeover ? takeoverType : manta[whichInst].type;
		int whichHex = (takeover || (hexagon < 16)) ? hexagon : (hexagon - 16);
		
		if (type == KeyboardInstrument)
		{
			if (whichHex == 8)
			{
				// Save
				Write7Seg(preset_num);
				
				displayState = GlobalDisplayStateNil;
				
				tKeyboard_hexmapEncode(hexmapEditKeyboard, hexmapBuffer);
				
				initiateStoringHexmapToExternalMemory(currentHexmapSelect);
			}
			else if (whichHex == 9)
			{
				// Load
				Write7Seg(preset_num);
				
				displayState = GlobalDisplayStateNil;
				
				initiateLoadingHexmapFromExternalMemory(currentHexmapSelect);
			}
		}
		else if (type == DirectInstrument)
		{
			if (whichHex == 8)
			{
				// Save
				Write7Seg(preset_num);
				
				displayState = GlobalDisplayStateNil;
				
				tDirect_encode(editDirect, directBuffer);
				
				initiateStoringDirectToExternalMemory(currentDirectSelect);
			}
			else if (whichHex == 9)
			{
				// Load
				Write7Seg(preset_num);
				
				displayState = GlobalDisplayStateNil;
				
				initiateLoadingDirectFromExternalMemory(currentDirectSelect);
			}
		}
		else if (type == SequencerInstrument)
		{
			if (whichHex < 13)
			{
				// Load Sequencer 
				Write7Seg(preset_num);
				
				displayState = GlobalDisplayStateNil;
				
				whichCompositionInstrument = whichInst;
				whichCompositionHex = whichHex;
				
				if (currentSequencerSelect >= 0)
				{
					initiateLoadingSequencerFromExternalMemory(currentSequencerSelect);
				}
			}
		}
		
        setCompositionLEDs();
		setHexmapConfigureLEDs();
		setDirectOptionLEDs();
    }
    else if (shiftOption2)
    {
		if (hexagon == 0)
		{
			manta_set_LED_hex(0, Red);
		}
    }
}

int lastTouch = 0;

void touchLowerHex(uint8_t hexagon)
{
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	
	if (manta[currentInstrument].type == SequencerInstrument)
	{
		if (sequencer->playMode == TouchMode)
		{
			jumpToStep(currentInstrument, hexagon);
			
			if (edit_vs_play == PlayToggleMode)
			{
				manta_set_LED_hex(hexagon, Amber);
				if (sequencer->lastTouch != hexagon) manta_set_LED_hex(sequencer->lastTouch, Off);
			}
			
			sequencer->lastTouch = hexagon;
			
			if (edit_vs_play == PlayToggleMode) return;	
		}
		
		if ((full_vs_split == SplitMode) && (edit_vs_play == TrigToggleMode) && ((hexagon < 16 && trigSelectOn >= 12) || (hexagon >= 16 && trigSelectOn < 4)))
		{
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
			
			for (int i = 0; i < 16; i++) manta_set_LED_hex(i+MAX_STEPS, Off);
			if ((manta[InstrumentOne].type == SequencerInstrument) && (manta[InstrumentTwo].type == SequencerInstrument) &&
			(manta[InstrumentOne].sequencer.pitchOrTrigger == TriggerMode) && (manta[InstrumentTwo].sequencer.pitchOrTrigger == TriggerMode))
			{
				setKeyboardLEDsFor(InstrumentOne, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
				setKeyboardLEDsFor(InstrumentTwo, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
			}
			else
			{
				setKeyboardLEDsFor(currentInstrument, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
			}
			
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
		
		sequencer = &manta[currentInstrument].sequencer;
		
		uint8_t uiHexCurrentStep = stepToHexUI(currentInstrument, manta[currentInstrument].sequencer.currentStep);

		uint8_t uiHexPrevStep = stepToHexUI(currentInstrument, manta[currentInstrument].sequencer.prevStep);
		
		int step = hexUIToStep(hexagon);
		
		if (edit_vs_play == EditMode)
		{
			if (editNoteOn >= 0)
			{
				// If hex is not in editStack, add it
				if (tNoteStack_contains(&editStack, hexagon) < 0)
				{
					tNoteStack_add(&editStack, hexagon);
					manta_set_LED_hex(hexagon, firstEdition ? Amber : Red);
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
				manta_set_LED_hex(editNoteOn, firstEdition ? Amber : Red);
			}

		}
		else if (edit_vs_play == PlayToggleMode)
		{
			tNoteStack_remove(&editStack, prevHexUI);
			tNoteStack_add(&editStack, currentHexUI);
			
			if (sequencer->playMode == ToggleMode) // note ons should toggle sequencer steps in and out of the pattern
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
						manta_set_LED_hex(hexagon, firstEdition ? Off : RedOff);
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

		if (!shiftOption1)
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
			if (trigSelectOn < 4)	setTriggerPanelLEDsFor(InstrumentOne,currentPanel[InstrumentOne]);
			else					setTriggerPanelLEDsFor(InstrumentTwo,currentPanel[InstrumentTwo]);
		}
		else // FullMode
		{
			setTriggerPanelLEDsFor(currentInstrument,currentPanel[currentInstrument]);
		}
		
	}
	else if (mode == PlayToggleMode)
	{
		if (full_vs_split == SplitMode)
		{
			setSequencerLEDsFor(InstrumentOne);
			setSequencerLEDsFor(InstrumentTwo);
			
			setKeyboardLEDsFor(currentInstrument, -1);
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
		
		if (!shiftOption1)
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
		int whichUpperHex = currentUpperHexUI - MAX_STEPS;
		MantaInstrument whichInst = ((whichUpperHex < 4) || (whichUpperHex >= 8 && whichUpperHex < 12)) ? InstrumentOne : InstrumentTwo;

		if (manta[whichInst].type != SequencerInstrument) return;
		
		sequencer = &manta[whichInst].sequencer;
		
		int whichTrigPanel = trigger_pattern[whichUpperHex];
		int whichMute = (whichUpperHex%4);
		int whichPanel = (whichUpperHex%4);

		if (whichTrigPanel != SXTrigPanelNil)
		{
			if ((trigSelectOn+MAX_STEPS) == hexagon)
			{
				 trigSelectOn = -1;
				 
				 manta_set_LED_hex(hexagon, (sequencer->mute[whichMute] ? (firstEdition ? Off : BothOn) : Amber));
				 
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



void uiOff(void)
{
	for (int i = 0; i < 48; i++)
	{	
		manta_set_LED_hex(i ,Off);
	}
}

void releaseUpperHexOptionMode(uint8_t hexagon)
{

}

int testnumvoices;
void touchUpperHexOptionMode(uint8_t hexagon)
{
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = takeover ? &fullKeyboard : &manta[currentInstrument].keyboard;
	tDirect* direct = takeover ? &fullDirect :&manta[currentInstrument].direct;
	
	MantaInstrumentType type = takeover ? takeoverType : manta[currentInstrument].type;
	
	uint8_t whichHex = hexagon - MAX_STEPS;
	
	OptionType* options =	(currentOptionMode == SequencerOptionMode) ? sequencerOptionMode :
	(currentOptionMode == KeyboardOptionMode) ? keyboardOptionMode :
	(currentOptionMode == DirectOptionMode) ? directOptionMode :
	(currentOptionMode == RightOptionMode) ? rightOptionMode :
	defaultOptionMode;
	
	OptionType whichOptionType =	options[whichHex];
									
	if (whichOptionType <= OptionPatternEight)
	{
		if (type == SequencerInstrument)		
		{
			if (whichOptionType != sequencer->pattern)
			{
				sequencer->reverse = FALSE;
			}
			else
			{
				sequencer->reverse = (sequencer->reverse ? FALSE : TRUE);
			}
			
			tSequencer_setPattern(sequencer, whichOptionType);
		}
		else if (type == KeyboardInstrument)	
		{
			tKeyboard_setArpModeType(keyboard, whichOptionType);
		}
		
		prev_pattern_hex = current_pattern_hex;
		current_pattern_hex = whichHex;
	}
	else if (whichOptionType == OptionSequencer)
	{
		resetEditStack();
		for (int i = 0; i < 6; i++) sendDataToOutput(i+currentInstrument*6,5,0);
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		takeover = FALSE;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;

		if (type != SequencerInstrument)
		{
			manta[currentInstrument].type = SequencerInstrument;
			manta[currentInstrument].sequencer.pitchOrTrigger = PitchMode;
			tIRampSetTime(&out[currentInstrument][CVTRIGGER], 0);
		}
	}
	else if (whichOptionType == OptionKeyboard)
	{
		resetEditStack();
		for (int i = 0; i < 6; i++) sendDataToOutput(i+currentInstrument*6,5,0);
		
		full_vs_split = FALSE;
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;
		
		if (type != KeyboardInstrument)
		{
			takeover = FALSE;
			manta[currentInstrument].type = KeyboardInstrument;
			
			keyboard->numVoices = 1;
			tIRampSetTime(&out[currentInstrument][CVPITCH], globalPitchGlide);
			tIRampSetTime(&out[currentInstrument][CVTRIGGER], 0);
		}
	}
	else if (whichOptionType == OptionDirect)
	{
		resetEditStack();
		for (int i = 0; i < 6; i++) sendDataToOutput(i+currentInstrument*6,5,0);
		
		full_vs_split = FALSE;
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;
		
		if (type != DirectInstrument)
		{
			takeover = FALSE;
			manta[currentInstrument].type = DirectInstrument;
			
			tIRampSetTime(&out[currentInstrument][0], 0);
			tIRampSetTime(&out[currentInstrument][1], 0);
			tIRampSetTime(&out[currentInstrument][2], 0);
			tIRampSetTime(&out[currentInstrument][3], 0);
			tIRampSetTime(&out[currentInstrument][4], 0);
			tIRampSetTime(&out[currentInstrument][5], 0);
		}
	}
	else if (whichOptionType == OptionPitch)
	{
		resetEditStack();
		for (int i = 0; i < 6; i++) sendDataToOutput(i+currentInstrument*6,5,0);
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		takeover = FALSE;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;

		if (sequencer->pitchOrTrigger != PitchMode)
		{
			sequencer->pitchOrTrigger = PitchMode;
			tNoteStack_init(&sequencer->notestack, 32);
			for (int i = 0; i < 32; i++) sequencer->step[i].toggled = 0;
			
			tIRampSetTime(&out[currentInstrument][CVTRIGGER], 0);
		}
		
	}
	else if (whichOptionType == OptionTrigger)
	{
		resetEditStack();
		for (int i = 0; i < 6; i++) sendDataToOutput(i+currentInstrument*6,5,0);
		
		prev_option_hex = current_option_hex;
		current_option_hex = whichHex;
		
		takeover = FALSE;
		
		currentHexUI = 0;
		
		currentUpperHexUI = 0;

		if (sequencer->pitchOrTrigger != TriggerMode)
		{
			sequencer->pitchOrTrigger = TriggerMode;
			tNoteStack_init(&sequencer->notestack, 32);
			for (int i = 0; i < 32; i++) sequencer->step[i].toggled = 0;

			//now set the ramps for the trigger outputs to be time zero so they are fast
			tIRampSetTime(&out[currentInstrument][CV1T], 0);
			tIRampSetTime(&out[currentInstrument][CV2T], 0);
			tIRampSetTime(&out[currentInstrument][CVTRIG1], 0);
			tIRampSetTime(&out[currentInstrument][CVTRIG2], 0);
			tIRampSetTime(&out[currentInstrument][CVTRIG3], 0);
			tIRampSetTime(&out[currentInstrument][CVTRIG4], 0);
		}
		
	}
	else if (whichOptionType == OptionEditType)
	{
		EditModeType type = sequencer->editType;
		
		type =	(type == NormalEdit) ? SubtleEdit : 
				(type == SubtleEdit) ? RandomEdit :
				(type == RandomEdit) ? NormalEdit :
				NormalEdit;
				
		sequencer->editType = type;
	}
	else if (whichOptionType == OptionFullSplit)
	{
		full_vs_split = (full_vs_split == FullMode) ? SplitMode : FullMode;
	}
	else if (!takeover && whichOptionType == OptionLeft)
	{
		prev_panel_hex = current_panel_hex;
		current_panel_hex = whichHex;
		
		setCurrentInstrument(InstrumentOne);
	}
	else if (!takeover && whichOptionType == OptionRight)
	{
		prev_panel_hex = current_panel_hex;
		current_panel_hex = whichHex;
		
		setCurrentInstrument(InstrumentTwo);
	}
	else if (whichOptionType == OptionMono)
	{
		if (takeover)
		{
			for (int i = 0; i < 12; i++) sendDataToOutput(i,5,0);
			
			takeover = FALSE;
			
			tKeyboard* keyboard = &manta[currentInstrument].keyboard;
			
			keyboard->numVoices = 1;
			tIRampSetTime(&out[currentInstrument][CVPITCH], globalPitchGlide);
			tIRampSetTime(&out[currentInstrument][CVTRIGGER], 0);
		}
	}
	else if (whichOptionType == OptionDuo)
	{
		for (int i = 0; i < 12; i++) sendDataToOutput(i,5,0);
		
		initMantaKeys(2);
	}
	else if (whichOptionType == OptionTrio)
	{
		for (int i = 0; i < 12; i++) sendDataToOutput(i,5,0);
		
		initMantaKeys(3);
	}
	else if (whichOptionType == OptionQuad)
	{
		for (int i = 0; i < 12; i++) sendDataToOutput(i,5,0);
		
		initMantaKeys(4);		
	}
	else if (whichOptionType == OptionSixOut)
	{
		takeover = FALSE;
		
		for (int i = 0; i < 6; i++)
		{
			tIRampSetTime(&out[currentInstrument][i], 0);
		}
	}
	else if (whichOptionType == OptionTwelveOut)
	{
		takeover = TRUE;
		takeoverType = DirectInstrument;

		for (int inst = 0; inst < 2; inst++)
		{
			for (int i = 0; i < 6; i++)
			{
				tIRampSetTime(&out[inst][i], 0);
			}
		}
		
	}
	else if (whichOptionType == OptionToggle)
	{
		sequencer->playMode = ToggleMode;
	}
	else if (whichOptionType == OptionArp)
	{
		sequencer->playMode = ArpMode;
		clearSequencer(currentInstrument);
	}
	else if (whichOptionType == OptionTouch)
	{
		sequencer->playMode = TouchMode;
		clearSequencer(currentInstrument);
	}
	else if (whichOptionType == OptionKeyArp)
	{
		if (keyboard->playMode == TouchMode)
		{
			keyboard->playMode = ArpMode;
		}
		else if (keyboard->playMode == ArpMode)
		{
			keyboard->playMode = TouchMode;
		}
	}
	
	setOptionLEDs();
	
	if (shiftOption1)
	{
		setDirectOptionLEDs();
		setCompositionLEDs();
		setHexmapConfigureLEDs();
	}
	else 
	{
		setTuningLEDs();
	}
	
	//set memory variables
	newUpperHexSeq = 0;
}

void touchUpperHex(uint8_t hexagon)
{
	prevUpperHexUI = currentUpperHexUI;
	currentUpperHexUI = hexagon;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	
	EditModeType editType = sequencer->editType;
	int random; 
	
	if (!shiftOption1)
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
					subtleInterval = (upperHexType > 0) ? upperHexType : 12;
					setParameterForEditStackSteps(currentInstrument, Pitch, upperHexType);

					if (sequencer->editType == NormalEdit) setParameterForEditStackSteps(currentInstrument, Octave, sequencer->octave);

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
				//comment this out if we don't want immediate DAC update, but only update at the beginning of a step
				
				DAC16Send(2 * currentInstrument, get16BitPitch(&myGlobalTuningTable, currentInstrument, cStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
			}
				

			setKeyboardLEDsFor(currentInstrument, (edit_vs_play == EditMode) ? hexUIToStep(tNoteStack_first(&editStack)) : -1);
		}
		else // TriggerMode
		{
			int whichUpperHex = currentUpperHexUI - MAX_STEPS;
			MantaInstrument whichInst = ((whichUpperHex < 4) || (whichUpperHex >= 8 && whichUpperHex < 12)) ? InstrumentOne : InstrumentTwo;

			if (manta[whichInst].type != SequencerInstrument || (manta[whichInst].sequencer.pitchOrTrigger != TriggerMode)) return;
			
			sequencer = &manta[whichInst].sequencer;
			
			int whichTrigPanel = trigger_pattern[whichUpperHex];
			int whichMute = (whichUpperHex%4);
			int whichPanel = (whichUpperHex%4);

			if (whichTrigPanel != SXTrigPanelNil)
			{
				// InstrumentOne Trig
				if (edit_vs_play == PlayToggleMode) 
				{
					trigSelectOn = whichUpperHex;
					
					manta_set_LED_hex(currentUpperHexUI, Red);
					
					currentPanel[currentInstrument] = whichPanel;
					
					switchToMode(TrigToggleMode);
				}
			}
			else
			{
				sequencer->mute[whichMute] = (sequencer->mute[whichMute] ? FALSE : TRUE);
				
				if (sequencer->mute[whichMute])
				{
					manta_set_LED_hex(currentUpperHexUI, Red);
					manta_set_LED_hex(currentUpperHexUI + ((whichInst == InstrumentOne) ? -8 : 8), firstEdition ? Off : BothOn);
				}
				else 
				{
					manta_set_LED_hex(currentUpperHexUI, Off);
					manta_set_LED_hex(currentUpperHexUI + ((whichInst == InstrumentOne) ? -8 : 8), ((editStack.size == 1) && sequencer[whichInst].step[editStack.notestack[0]].on[whichMute]) ? Red : Amber);
				}
			}
		
			if (whichInst != currentInstrument)
			{
				setCurrentInstrument(whichInst);
				
				if (edit_vs_play != TrigToggleMode)
				{
					setKeyboardLEDsFor(currentInstrument, (edit_vs_play == EditMode) ? hexUIToStep(tNoteStack_first(&editStack)) : -1);
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
	if (!takeover)
	{
		freeze_LED_update = 1;
	
		if (full_vs_split == FullMode)
		{
			setSequencerLEDsFor(currentInstrument);
		}
		else
		{
			setSequencerLEDsFor(InstrumentOne);
			setSequencerLEDsFor(InstrumentTwo);
		}
		
		for (int i = 0; i < 16; i++) manta_set_LED_hex(i+MAX_STEPS, Off);
		
		if ((manta[InstrumentOne].type == SequencerInstrument) && (manta[InstrumentTwo].type == SequencerInstrument) &&
			(manta[InstrumentOne].sequencer.pitchOrTrigger == TriggerMode) && (manta[InstrumentTwo].sequencer.pitchOrTrigger == TriggerMode))
		{
			setKeyboardLEDsFor(InstrumentOne, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
			setKeyboardLEDsFor(InstrumentTwo, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
		}
		else 
		{
			setKeyboardLEDsFor(currentInstrument, ((edit_vs_play == TrigToggleMode) ? 0 : -1));
		}

		roll_LEDs = 1;
		freeze_LED_update = 0;
	}

}

void setKeyboardLEDs(void)
{
	freeze_LED_update = 1;
	
	if (!takeover)
	{
		if (manta[currentInstrument].type == KeyboardInstrument)
		{
			tKeyboard* keyboard = &manta[currentInstrument].keyboard;

			for (int i = 0; i < 48; i++)
			{
				manta_set_LED_hex(i, keyboard->hexes[i].color);
			}
		}
	}
	else if (takeoverType == KeyboardInstrument)
	{
		tKeyboard* keyboard = &fullKeyboard;
		
		for (int i = 0; i < 48; i++)
		{
			manta_set_LED_hex(i, keyboard->hexes[i].color);
		}
		
	}
	roll_LEDs = 1;
	freeze_LED_update = 0;

}

void setDirectLEDs			(void)
{
	freeze_LED_update = 1;
	if (!takeover)
	{
		if (manta[currentInstrument].type == DirectInstrument)
		{
			tDirect* direct = &manta[currentInstrument].direct;
			
			MantaLEDColor color = Off;
			DirectType type = DirectTypeNil;			
			for (int i = 0; i < 48; i++)
			{
				type = direct->hexes[i].type;
				
				color = (type == DirectCV) ? Amber :
				(type == DirectGate) ? (firstEdition ? Amber : Red) :
				(type == DirectTrigger) ? (firstEdition ? Amber : BothOn) :
				Off;
				
				manta_set_LED_hex(i, color);
			}
			
			for (int i = 0; i < 2; i++)
			{
				type = direct->sliders[i].type;
				
				if (type == DirectCV)
				{
					manta_set_LED_slider(i, ((direct->sliders[i].value >> 9) + 1));
					manta_set_LED_button(i ? ButtonTopRight : ButtonTopLeft, (firstEdition ? Amber : Red));
				}
				else
				{
					manta_set_LED_slider(i, 0);
					manta_set_LED_button(i ? ButtonTopRight : ButtonTopLeft, Off);
				}
			}

		}
	}
	else if (takeoverType == DirectInstrument)
	{
		manta_clear_all_LEDs();
		
		MantaLEDColor color = Off;
		DirectType type = DirectTypeNil;
		
		for (int i = 0; i < 48; i++)
		{
			type = fullDirect.hexes[i].type;
			
			color = (type == DirectCV) ? Amber :
			(type == DirectGate) ? (firstEdition ? Amber : Red) :
			(type == DirectTrigger) ? (firstEdition ? Amber : BothOn) :
			Off;
			
			manta_set_LED_hex(i, color);
		}
		
		for (int i = 0; i < 2; i++)
		{
			type = fullDirect.sliders[i].type;
			
			if (type == DirectCV)
			{
				manta_set_LED_slider(i, ((fullDirect.sliders[i].value >> 9) + 1));
				manta_set_LED_button(i ? ButtonTopRight : ButtonTopLeft, (firstEdition ? Amber : Red));
			}
			else
			{
				manta_set_LED_slider(i, 0);
				manta_set_LED_button(i ? ButtonTopRight : ButtonTopLeft, Off);
			}
		}
	}
	roll_LEDs = 1;
	freeze_LED_update = 0;
	
}

void setDirectOptionLEDs			(void)
{
	freeze_LED_update = 1;
	if (!takeover)
	{
		for (int inst = 0; inst < 2; inst++)
		{
			if (manta[inst].type == DirectInstrument)
			{
				for (int i = 0; i < 16; i++) manta_set_LED_hex(16*inst+i,Off);

				manta_set_LED_hex(16*inst + 8, (firstEdition ? Amber : Red)); // SAVE maybe do this for direct too?
				manta_set_LED_hex(16*inst + 9, (firstEdition ? Amber : Red)); // LOAD maybe do this for direct too?
				manta_set_LED_hex(16*inst + 11, (firstEdition ? Amber : Red)); // EDIT
				
				manta_set_LED_hex(16*inst + 0, Amber); // Default
				manta_set_LED_hex(16*inst + 1, Amber); // All Triggers
				manta_set_LED_hex(16*inst + 2, Amber); // All Gates
				manta_set_LED_hex(16*inst + 3, Amber); // All CVs
				manta_set_LED_hex(16*inst + 4, Amber); // Trigger/CV
				manta_set_LED_hex(16*inst + 7, (firstEdition ? Amber : Red)); // BLANK
			}
			
		}
	}
	else if (takeoverType == DirectInstrument)
	{
		for (int i = 0; i < 32; i++) manta_set_LED_hex(i,Off);
		
		manta_set_LED_hex(8, (firstEdition ? Amber : Red)); // SAVE maybe do this for direct too?
		manta_set_LED_hex(9, (firstEdition ? Amber : Red)); // LOAD maybe do this for direct too?
		manta_set_LED_hex(11, (firstEdition ? Amber : Red)); // EDIT
		
		manta_set_LED_hex(0, Amber); // Default
		manta_set_LED_hex(1, Amber); // All Triggers
		manta_set_LED_hex(2, Amber); // All Gates
		manta_set_LED_hex(3, Amber); // All CVs
		manta_set_LED_hex(4, Amber); // Trigger/CV
		manta_set_LED_hex(7, (firstEdition ? Amber : Red)); // BLANK
	}
	
	roll_LEDs = 1;
	freeze_LED_update = 0;
}

// ~ ~ ~ ~ TOP LEFT BUTTON ~ ~ ~ ~ //
void touchTopLeftButton(void)
{
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	
	MantaInstrumentType type = manta[currentInstrument].type;
	
	if (type == SequencerInstrument)
	{
		if (shiftOption1)
		{
			shiftOption1SubShift = SubShiftTopLeft;
		}
		else if (shiftOption2)
		{
			sequencer->transpose -= 1;
			indicateTransposition(sequencer->transpose);
		}
		else
		{
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
			else 
			{
				if (glideNoteOn == -1)
				{
					glideNoteOn = 1;
					
					// Enter SliderModeGlide
					prevMantaSliderMode = currentMantaSliderMode;
					currentMantaSliderMode = SliderModeGlide;
					
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
	
		}
		
	}
	else if (type == KeyboardInstrument)
	{
		if (shiftOption2)
		{
			keyboard->transpose -= 1;
			indicateTransposition(keyboard->transpose);
		}
		else
		{
			keyboard->transpose -= myGlobalTuningTable.cardinality; //fix this so it's the current tuning table
			indicateTransposition(keyboard->transpose);
		}
		
		dacSendKeyboard(currentInstrument);
	}
	else // DirectInstrument
	{

	}
	
	
	
}

void releaseTopLeftButton(void)
{
	shiftOption1SubShift = SubShiftNil;	
	
	if (manta[currentInstrument].type == SequencerInstrument)
	{
		if (manta[currentInstrument].sequencer.pitchOrTrigger == TriggerMode)
		{
			if (glideNoteOn == 1)
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
			}
		}
		
	}
	
}

// ~ ~ ~ ~ TOP RIGHT BUTTON ~ ~ ~ ~ //
void touchTopRightButton(void)
{
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = &manta[currentInstrument].keyboard;
	
	if (manta[currentInstrument].type == SequencerInstrument)
	{
		if (shiftOption1)
		{
			shiftOption1SubShift = SubShiftTopRight;
			
			displayState = SequencerSelect;
			
			currentSequencerSelect = -1;
			Write7Seg(currentSequencerSelect);
		}
		else if (shiftOption2)
		{
			sequencer->transpose += 1;
			indicateTransposition(sequencer->transpose);
		}
		else
		{
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
	}
	else
	{
		// Tranpose up 
		if (shiftOption2)
		{
			keyboard->transpose += 1;
			indicateTransposition(keyboard->transpose);
		}
		else
		{
			keyboard->transpose += myGlobalTuningTable.cardinality; //fix this so that it's the currently used tuning table instead
			indicateTransposition(keyboard->transpose);
		}
		
		dacSendKeyboard(currentInstrument);
	}
}

void releaseTopRightButton(void)
{
	if (!takeover && (manta[currentInstrument].type == SequencerInstrument))
	{
		if (shiftOption1SubShift == SubShiftTopRight)
		{
			// Save Sequencer
			Write7Seg(preset_num);
			
			displayState = GlobalDisplayStateNil;
			
			if (currentSequencerSelect >= 0)
			{
				tSequencer_encode(&manta[InstrumentOne].sequencer, sequencerBuffer);
				tSequencer_encode(&manta[InstrumentTwo].sequencer, &sequencerBuffer[NUM_BYTES_PER_SEQUENCER]);
				
				initiateStoringSequencerToExternalMemory(currentSequencerSelect);
			}
		}
		shiftOption1SubShift = SubShiftNil;
		copyStage = 0;
		copyWhichInst = InstrumentNil;
		copyWhichComp = -1;
		
		if (shiftOption1)	setCompositionLEDs();
		else				setSequencerLEDs();
	}
}

// ~ ~ ~ ~ BOTTOM LEFT BUTTON ~ ~ ~ ~ //
void touchBottomLeftButton(void)
{
	if (hexmapEditMode || directEditMode)
	{
		hexmapEditMode = FALSE;
		directEditMode = FALSE;
		Write7Seg(preset_num);
		displayState = GlobalDisplayStateNil;
		
		setHexmapConfigureLEDs();
		setCompositionLEDs();
		setDirectOptionLEDs();
	}
	else
	{
		if (shiftOption2)
		{
			if (!shiftOption2Lock) 
			{
				shiftOption2Lock = TRUE;
				displayState = UpDownSwitchBlock;
				manta_set_LED_button(ButtonBottomLeft, Red);
				manta_set_LED_button(ButtonBottomRight, Amber);
			}
			else
			{
				shiftOption2Lock = FALSE;
				displayState = GlobalDisplayStateNil;
			
				shiftOption2 = FALSE;
			
				Write7Seg(normal_7seg_number);
				transpose_indication_active = 0;
				if (!savingActive)
				{
					LED_Off(PRESET_SAVE_LED);
				}
				setCurrentInstrument(currentInstrument);
			
				manta_set_LED_button(ButtonBottomLeft, Off);
				manta_set_LED_button(ButtonBottomRight, Off);
			
				setSequencerLEDs();
				setKeyboardLEDs();
				setDirectLEDs();
			}
		}
		else //normal
		{
			if (shiftOption1Lock)
			{
				shiftOption1Lock = FALSE;
				displayState = GlobalDisplayStateNil;
			}
			
			shiftOption1 = TRUE;
		
			setOptionLEDs();
		
			if (!takeover)
			{
				setCompositionLEDs();
				setHexmapConfigureLEDs();
				setDirectOptionLEDs();
			}
			else if (takeoverType == KeyboardInstrument)
			{
				setHexmapConfigureLEDs();
			}
			else if (takeoverType == DirectInstrument)
			{
				setDirectOptionLEDs();
			}

			manta_set_LED_button(ButtonBottomLeft, Amber);
		}
	}
	
}


void releaseBottomLeftButton(void)
{	
	if (!shiftOption2 && !shiftOption1Lock)
	{
		shiftOption1 = FALSE;
		
		displayState = GlobalDisplayStateNil;
		
		Write7Seg(normal_7seg_number);
		transpose_indication_active = 0;
		if (!savingActive)
		{
			LED_Off(PRESET_SAVE_LED);
		}
		setCurrentInstrument(currentInstrument);
		
		setSequencerLEDs();
		setKeyboardLEDs();
		setDirectLEDs();
	}
	
}

void setTuningLEDs(void)
{

	MantaInstrumentType type1 = manta[InstrumentOne].type;
	MantaInstrumentType type2 = manta[InstrumentTwo].type;
	
	if ((takeover && takeoverType == KeyboardInstrument) || (type1 == SequencerInstrument) || (type1 == KeyboardInstrument) || (type2 == SequencerInstrument) || (type2 == KeyboardInstrument))
	{
		for (int i = 1; i < 32; i++)
		{
			manta_set_LED_hex(i, (i == currentTuningHex) ? Amber : Off);
		}
		
		manta_set_LED_hex(0, firstEdition ? Amber : Red); // Return to Global tuning (globalTuning)
	}

	
}

// ~ ~ ~ ~ BOTTOM RIGHT BUTTON ~ ~ ~ ~ //
void touchBottomRightButton(void)
{

	if (shiftOption1)
	{
		if (!shiftOption1Lock)
		{
			shiftOption1Lock = TRUE;
			lastDirectEditHex = -1;
			displayState = UpDownSwitchBlock;
			manta_set_LED_button(ButtonBottomLeft, Amber);
			manta_set_LED_button(ButtonBottomRight, Red);
		}
		else
		{
			shiftOption1Lock = FALSE;
			displayState = GlobalDisplayStateNil;
			
			shiftOption1 = FALSE;
			
			
			Write7Seg(normal_7seg_number);
			transpose_indication_active = 0;
			if (!savingActive)
			{
				LED_Off(PRESET_SAVE_LED);
			}
			setCurrentInstrument(currentInstrument);
			
			manta_set_LED_button(ButtonBottomLeft, Off);
			manta_set_LED_button(ButtonBottomRight, Off);
			
			setSequencerLEDs();
			setKeyboardLEDs();
			setDirectLEDs();
		}
	}
	else
	{
		if (!shiftOption2Lock)
		{
			shiftOption2 = TRUE;
			
			displayState = UpDownSwitchBlock;
			
			setOptionLEDs();
			
			setTuningLEDs();
			
			manta_set_LED_button(ButtonBottomRight, Amber);
		}
		else
		{
			shiftOption2Lock = FALSE;
			displayState = GlobalDisplayStateNil;
			
			shiftOption2 = TRUE;
			
			displayState = GlobalDisplayStateNil;
			
			Write7Seg(normal_7seg_number);
			transpose_indication_active = 0;
			if (!savingActive)
			{
				LED_Off(PRESET_SAVE_LED);
			}
			setCurrentInstrument(currentInstrument);
			
			setTuningLEDs();
		}
	}

}

void releaseBottomRightButton(void)
{	
	if (shiftOption1)	
	{
		setCompositionLEDs();
	}	
	else if (!shiftOption2Lock)
	{	
		shiftOption2 = FALSE;
		
		displayState = GlobalDisplayStateNil;
		
		Write7Seg(normal_7seg_number);
		transpose_indication_active = 0;
		if (!savingActive)
		{
			LED_Off(PRESET_SAVE_LED);
		}
		setCurrentInstrument(currentInstrument);
		
		setSequencerLEDs();
		setKeyboardLEDs();
		setDirectLEDs();
	}

}

uint16_t testval = 0;
void processSliderSequencer(uint8_t sliderNum, uint16_t val)
{
	if (manta[currentInstrument].type != SequencerInstrument) return;
	
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	EditModeType editType = sequencer->editType;
	
	int currStep = sequencer->currentStep;
	
	int uiHexCurrentStep = stepToHexUI(currentInstrument, currStep);

	if (currentMantaSliderMode == SliderModeOne)
	{
		// Set proper internal state
		if (sliderNum == SliderOne)
		{
			setParameterForEditStackSteps(currentInstrument, CV1, val);
		}
		else // SliderTwo
		{
			setParameterForEditStackSteps(currentInstrument, CV2, val);
		}
		
		manta_set_LED_slider(sliderNum, (val >> 9) + 1); // add one to the slider values because a zero turns them off

		if (tNoteStack_contains(&editStack, uiHexCurrentStep) != -1)
		{
			if (sliderNum == SliderOne)
			{
				//dacsend(currentInstrument * 2, 1, val); need to replace this with RAMP update TODO:
			}
			else
			{
				//DAC16Send(2*currentInstrument+1, val << 4);need to replace this with RAMP update TODO:
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
				//dacsend(currentInstrument * 2 + 1, 0, val); need to replace this with RAMP update TODO:
			}
			else
			{
				// dacsend(currentInstrument * 2 + 1, 1, val); need to replace this with RAMP update TODO:
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
				tIRampSetDest(&out[currentInstrument][CVPITCH], get16BitPitch(&myGlobalTuningTable, currentInstrument, currStep));
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
				DAC16Send(2 * currentInstrument, get16BitPitch(&myGlobalTuningTable, currentInstrument, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
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
				DAC16Send(2 * currentInstrument, get16BitPitch(&myGlobalTuningTable, currentInstrument, currStep)); // take pitch class, add octave * 12, multiply it by the scalar, divide by 1000 to get 16 bit.
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
	if (shiftOption1 || shiftOption2) return;
	
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
				manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Off : AmberOn);
			}
			else
			{
				manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Amber : RedOn);
			}
			
			if (tNoteStack_contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, firstEdition ? Amber : AmberOff);
			}
			else
			{
				manta_set_LED_hex(uiHexPrevStep, firstEdition ? Off : RedOff);
			}
		}
		else // TriggerMode
		{
			if (tNoteStack_contains(&editStack,uiHexCurrentStep) >= 0)
			{
				manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Off : AmberOn);
			}
			else
			{
				if (sequencer->step[cStep].on[currentPanel[currentInstrument]])
				{
					manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Amber : Red);
				}
				else
				{
					manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Amber : RedOn);
				}
			}
			
			if (tNoteStack_contains(&editStack,uiHexPrevStep) >= 0)
			{
				manta_set_LED_hex(uiHexPrevStep, firstEdition ? Amber : AmberOff);
			}
			else
			{
				if (sequencer->step[pStep].on[currentPanel[currentInstrument]])
				{
					manta_set_LED_hex(uiHexPrevStep, firstEdition ? Off : Amber);
				}
				else
				{
					manta_set_LED_hex(uiHexPrevStep,firstEdition ? Off : RedOff);
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
				manta_set_LED_hex(uiHexPrevStep,  Amber);
			}

			if (sequencer->step[cStep].toggled)
			{
				if (!sequencer->step[cStep].on[currentPanel[currentInstrument]])
				{
					manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Off : RedOn);
				}
				else
				{
					manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Off : Red);
				}
			}
			
		}
		else
		{
			manta_set_LED_hex(uiHexPrevStep, firstEdition ? Amber : RedOff);
			if (sequencer->notestack.size > 0) manta_set_LED_hex(uiHexCurrentStep, firstEdition ? Off : RedOn);
		}
		
	}
	else // TrigToggleMode
	{
		manta_set_LED_hex(uiHexPrevStep,  AmberOff);
		manta_set_LED_hex(uiHexCurrentStep, AmberOn);
	}
}

void setPanelSelectLEDs(void)
{
	manta_set_LED_hex(currentPanel[InstrumentOne] + MAX_STEPS, Red);
	manta_set_LED_hex(currentPanel[InstrumentTwo] + MAX_STEPS + 12, Red);	
}

int pitchToKbdHex(int pitch)
{
	pitch = (pitch == 0) ? 0 :
			(pitch == 1) ? 8 :
			(pitch == 2) ? 1 :
			(pitch == 3) ? 9 :
			(pitch == 4) ? 2 :
			(pitch == 5) ? 3 :
			(pitch == 6) ? 11 :
			(pitch == 7) ? 4 :
			(pitch == 8) ? 12 :
			(pitch == 9) ? 5 :
			(pitch == 10) ? 13 :
			(pitch == 11) ? 6 :
			0;
		
	return (MAX_STEPS + pitch);
}

void setKeyboardLEDsFor(MantaInstrument inst, int note)
{	
	if (manta[inst].type != SequencerInstrument) return;
	
	freeze_LED_update = 1;

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
					manta_set_LED_hex(j+MAX_STEPS, (firstEdition ? Off : Red));
				}
				else if (keyboard_pattern[j] == KeyboardPanelGlide)
				{
					manta_set_LED_hex(j+MAX_STEPS, (glideNoteOn >= 0) ? (firstEdition ? Amber : Red) : Off);
				}
				else
				{
					manta_set_LED_hex(j+MAX_STEPS, Off);
				}
			}
			
			if (setRed)
			{
				manta_set_LED_hex(pitchToKbdHex(sequencer->step[nt].pitch) , Red);
			}
		}
		else
		{
			for (int j = 0; j < 16; j++)
			{
				if (keyboard_pattern[j] < KeyboardEndOctave)
				{
					manta_set_LED_hex(j+MAX_STEPS, firstEdition ? Off : Red);
				}
				else if (keyboard_pattern[j] == KeyboardPanelRest)
				{
					manta_set_LED_hex(j+MAX_STEPS, Amber);
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
			int hex = i+MAX_STEPS;
			int which = trigger_pattern[i];
			int whichMute = i % 4;
			
			if ((inst == InstrumentOne && which == S1TrigPanelSelect) || (inst == InstrumentTwo && which == S2TrigPanelSelect))
			{
				if (sequencer->mute[whichMute]) 
				{
					manta_set_LED_hex(hex, firstEdition ? Off : BothOn);
					manta_set_LED_hex(hex + ((inst == InstrumentOne) ? 8 : -8), Red);
				}
				else
				{
					manta_set_LED_hex(hex, Amber);
					manta_set_LED_hex(hex + ((inst == InstrumentOne) ? 8 : -8), Off);
				}
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
	
	freeze_LED_update = 0;
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
	freeze_LED_update = 1;
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int hexUI = 0;
	for (int i = 0; i < sequencer->maxLength; i++)
	{
		hexUI = stepToHexUI(inst,i);
		
		manta_set_LED_hex(hexUI,Off);
		
		if (sequencer->step[i].on[(int)panel] > 0)
		{
			manta_set_LED_hex(hexUI, (i == sequencer->currentStep) ? (firstEdition ? Off : BothOn) : (firstEdition ? Amber : Red));
		}
		else
		{
			manta_set_LED_hex(hexUI, Off);
		}
	}
	roll_LEDs = 1;
	freeze_LED_update = 0;
}

void setSequencerLEDsFor(MantaInstrument inst)
{
	freeze_LED_update = 1;
	tSequencer* sequencer = &manta[inst].sequencer;
	
	int hexUI = 0;

	for (int i = 0; i < ((full_vs_split == FullMode) ? 32 : 16); i++)
	{
		hexUI = stepToHexUI(inst, i);

		if(edit_vs_play == PlayToggleMode)
		{
			manta_set_LED_hex(hexUI, sequencer->step[i].toggled ? 
			(firstEdition ? (i == sequencer->currentStep ? Off : Amber) : 
			(i == sequencer->currentStep ? BothOn : Amber)) : Off);
		}
		else
		{
			manta_set_LED_hex(hexUI, sequencer->step[i].toggled ?
			(firstEdition ? (i == sequencer->currentStep ? Amber : Off) :
			(i == sequencer->currentStep ? BothOn : Amber)) : Off);
		}
	}
	
	int size = editStack.size;
	
	if (edit_vs_play == EditMode)
	{
		for (int i = 0; i < size; i++)
		{
			manta_set_LED_hex(editStack.notestack[i], firstEdition ? Amber : Red);
		}
	}
	roll_LEDs = 1;
	freeze_LED_update = 0;
}

void setOptionLEDs(void)
{
	freeze_LED_update = 1;
	MantaInstrumentType type = takeover ? takeoverType : manta[currentInstrument].type;
	MantaInstrumentType type1 = manta[InstrumentOne].type; MantaInstrumentType type2 = manta[InstrumentTwo].type;
	
	currentOptionMode =	(shiftOption2) ? (type ? DirectOptionMode : RightOptionMode) :
	(type == SequencerInstrument) ? SequencerOptionMode :
	(type == KeyboardOptionMode) ? KeyboardOptionMode :
	(type == DirectInstrument) ? DirectOptionMode :
	OptionModeNil;
	
	OptionType* options =		(currentOptionMode == SequencerOptionMode) ? sequencerOptionMode :
								(currentOptionMode == KeyboardOptionMode) ? keyboardOptionMode :
								(currentOptionMode == DirectOptionMode) ? directOptionMode :
								(currentOptionMode == RightOptionMode) ? rightOptionMode : 
								defaultOptionMode;
								
	tSequencer* sequencer = &manta[currentInstrument].sequencer;
	tKeyboard* keyboard = takeover ? &fullKeyboard :  &manta[currentInstrument].keyboard;
	
	int numVoices = 0;
	if (takeover)	numVoices = fullKeyboard.numVoices;
	else			numVoices = manta[currentInstrument].keyboard.numVoices;

	OptionType option = OptionNil;
	uint8_t hex = 64;
	for (int i = 0; i < 16; i++)
	{
		option = options[i];
		hex = i + MAX_STEPS;
		
		if (option == OptionSequencer)
		{
			if (type == SequencerInstrument)manta_set_LED_hex(hex, Red);
			else							manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionKeyboard)
		{
			if (type == KeyboardInstrument)	manta_set_LED_hex(hex, Red);
			else							manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionDirect)
		{
			if (type == DirectInstrument)	manta_set_LED_hex(hex, Red);
			else							manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionPitch)
		{
			if (sequencer->pitchOrTrigger == PitchMode)		manta_set_LED_hex(hex, Red);
			else											manta_set_LED_hex(hex, Amber);
			
		}
		else if (option == OptionTrigger)
		{
			if (sequencer->pitchOrTrigger == TriggerMode)	manta_set_LED_hex(hex, Red);
			else											manta_set_LED_hex(hex, Amber);
		}
		else if (option == OptionFullSplit)
		{
			if ((type1 == SequencerInstrument || type1 == DirectInstrument) && (type2 == SequencerInstrument || type2 == DirectInstrument))
				manta_set_LED_hex(hex, (full_vs_split == FullMode) ? Amber : Red);
			else
				manta_set_LED_hex(hex, Off);
		}
		else if (option <= OptionPatternEight)
		{
			manta_set_LED_hex(hex,	(type == SequencerInstrument) ? ((option == sequencer->pattern) ? (sequencer->reverse ? Red : BothOn) : Amber) : 
									(type == KeyboardInstrument) ? ((option == keyboard->arpModeType) ? Red : Amber) : Off);	
		}
		else if (!takeover && option == OptionLeft)
		{
			manta_set_LED_hex(hex, (currentInstrument == InstrumentOne) ? Red : Amber);
		}
		else if (!takeover && option == OptionRight)
		{
			manta_set_LED_hex(hex, (currentInstrument == InstrumentTwo) ? Red : Amber);
		}
		else if (option == OptionArp)
		{
			manta_set_LED_hex(hex, (sequencer->playMode == ArpMode) ? Red : Amber);
		}
		else if (option == OptionToggle)
		{
			manta_set_LED_hex(hex, (sequencer->playMode == ToggleMode) ? Red : Amber);
		}
		else if (option == OptionTouch)
		{
			manta_set_LED_hex(hex, (sequencer->playMode == TouchMode) ? Red : Amber);
		}
		else if (option == OptionEditType)
		{
			manta_set_LED_hex(hex,	(sequencer->editType == NormalEdit) ? Amber : 
									(sequencer->editType == RandomEdit) ? Red : 
									(sequencer->editType == SubtleEdit) ? BothOn :
									Off);
		}
		else if (option == OptionKeyArp)
		{
			manta_set_LED_hex(hex, (keyboard->playMode == ArpMode) ? Red : Amber);
		}
		else if (option == OptionMono)
		{
			manta_set_LED_hex(hex, (numVoices == 1) ? Red : Amber);
		}
		else if (option == OptionDuo)
		{
			manta_set_LED_hex(hex, (numVoices == 2) ? Red : Amber);
		}
		else if (option == OptionTrio)
		{
			manta_set_LED_hex(hex, (numVoices == 3) ? Red : Amber);
		}
		else if (option == OptionQuad)
		{
			manta_set_LED_hex(hex, (numVoices == 4) ? Red : Amber);
		}
		else if (option == OptionSixOut)
		{
			manta_set_LED_hex(hex, !takeover ? Red : Amber);
		}
		else if (option == OptionTwelveOut)
		{
			manta_set_LED_hex(hex, takeover ? Red : Amber);
		}
		else 
		{
			manta_set_LED_hex(hex, Off);
		}

	}
	roll_LEDs = 1;
	freeze_LED_update = 0;
}

// Output
uint32_t get16BitPitch(tTuningTable* myTable, MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;

	int32_t DACtemp = ((int32_t)sequencer->step[step].pitch) + sequencer->transpose;
	DACtemp += (sequencer->step[step].octave * myTable->cardinality);
	if (DACtemp > 127)
	{
		DACtemp = 127;
	}
	if (DACtemp < 0)
	{
		DACtemp = 0;
	}
	DACtemp = lookupDACvalue(myTable, (uint8_t)DACtemp, 0);
	DACtemp += (sequencer->step[step].fine >> 2) - 512;
	if (DACtemp < 0)
	{
		DACtemp = 0;
	}
	return ((uint32_t)DACtemp);
}

uint16_t glideTimeTEMP = 0;
float destTEMP= .0f;

void dacSendPitchMode(MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	if (sequencer->step[step].note)
	{
		uint16_t glideTime =  sequencer->step[step].pglide >> 3;
		if (glideTime < 1) glideTime = 1; //let's make it faster - was 5 - could be zero now but that would likely cause clicks
		
		tIRampSetDest(&out[inst][CVPITCH], get16BitPitch(&myGlobalTuningTable,inst,step)); 
		tIRampSetTime(&out[inst][CVPITCH], glideTime);

		// Configure CVGlide
		glideTime =  sequencer->step[step].cvglide >> 3;
		if (glideTime < 1) glideTime = 1; //let's make it faster - was 5  - could be zero now but that would likely cause clicks

		tIRampSetTime(&out[inst][CV1P], glideTime);
		tIRampSetDest(&out[inst][CV1P], sequencer->step[step].cv1);
		
		tIRampSetTime(&out[inst][CV2P], glideTime);
		tIRampSetDest(&out[inst][CV2P], sequencer->step[step].cv2 << 4);
		
		tIRampSetTime(&out[inst][CV3P], glideTime);
		tIRampSetDest(&out[inst][CV3P], sequencer->step[step].cv3);
		
		tIRampSetTime(&out[inst][CV4P], glideTime);
		tIRampSetDest(&out[inst][CV4P], sequencer->step[step].cv4);
		
		// Send Trigger
		tIRampSetTime(&out[inst][CVTRIGGER],0);
		tIRampSetDest(&out[inst][CVTRIGGER], 4095);
		sequencer->trigCount[0] = TRIGGER_TIMING; //start counting down for the trigger to turn off
	}
}


void dacSendTriggerMode(MantaInstrument inst, uint8_t step)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	// Configure CVGlide
	uint16_t glideTime =  sequencer->step[step].cvglide >> 3;
	if (glideTime < 5) glideTime = 5;

	tIRampSetTime(&out[inst][CV1T], glideTime);
	tIRampSetDest(&out[inst][CV1T], sequencer->step[step].cv1 << 4);
		
	tIRampSetTime(&out[inst][CV2T], glideTime);
	tIRampSetDest(&out[inst][CV2T], sequencer->step[step].cv2 << 4);

	// Trigger 1, Trigger 2, Trigger 3, Trigger 4
	if (!sequencer->mute[0]) 
	{
		tIRampSetDest(&out[inst][CVTRIG1],sequencer->step[step].on[0] * 4095);
		sequencer->trigCount[1] = sequencer->step[step].on[0] * TRIGGER_TIMING;
	}
	
	if (!sequencer->mute[1])
	{
		tIRampSetDest(&out[inst][CVTRIG2],sequencer->step[step].on[1] * 4095);
		sequencer->trigCount[2] = sequencer->step[step].on[1] * TRIGGER_TIMING;
	}
	
	if (!sequencer->mute[2])
	{
		tIRampSetDest(&out[inst][CVTRIG3],sequencer->step[step].on[2] * 4095);
		sequencer->trigCount[3] = sequencer->step[step].on[2] * TRIGGER_TIMING;
	}
	
	if (!sequencer->mute[3])
	{
		tIRampSetDest(&out[inst][CVTRIG4],sequencer->step[step].on[3] * 4095);
		sequencer->trigCount[4] = sequencer->step[step].on[3] * TRIGGER_TIMING;
	}
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
	else if (param == PitchGlide)	sequencer->step[step].pglide = value;
	else if (param == CVGlide)		sequencer->step[step].cvglide = value;
	else if (param == On1)			sequencer->step[step].on[PanelOne] = value;
	else if (param == On2)			sequencer->step[step].on[PanelTwo] = value;
	else if (param == On3)			sequencer->step[step].on[PanelThree] = value;
	else if (param == On4)			sequencer->step[step].on[PanelFour] = value;
	else;
}

#define SUBTLE_CV_AMT 50

void setParameterForEditStackSteps(MantaInstrument inst, StepParameterType param, uint16_t value)
{
	tSequencer* sequencer = &manta[inst].sequencer;
	
	EditModeType editType = sequencer->editType;
	
	int size = editStack.size;
	int i = 0;

	if (param == Toggled)
	{
		for (; i < size; i++) 
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].toggled =	value; // Random and RandomWalk (might be interesting to change only one or two steps in RandomWalk, or slowly shift steps that are toggled.
		}
	}
	else if (param == Length)
	{		
		for (; i < size; i++) 
		{
			value = (editType == RandomEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : 
					(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].length + ((rand() % 0xffff) > 0x8000) ? 1 : -1) : 
					value;
					
			sequencer->step[hexUIToStep(editStack.notestack[i])].length = value;
		}
	}
	else if (param == CV1)	
	{		
		for (; i < size; i++) 
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
					(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].cv1 + (((rand() / SIXTEEN_BIT_DIV) > 0x8000) ? SUBTLE_CV_AMT : -SUBTLE_CV_AMT)) :
					value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].cv1 = value;
		}
	}
	else if (param == CV2)	
	{		
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].cv2 + ((rand() % 0xffff) > 0x8000) ? SUBTLE_CV_AMT : -SUBTLE_CV_AMT) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].cv2 = value;
		}
	}
	else if (param == CV3)	
	{		
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].cv3 + ((rand() % 0xffff) > 0x8000) ? SUBTLE_CV_AMT : -SUBTLE_CV_AMT) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].cv3 = value;
		}
	}
	else if (param == CV4)	
	{		
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].cv4 + ((rand() % 0xffff) > 0x8000) ? SUBTLE_CV_AMT : -SUBTLE_CV_AMT) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].cv4 = value;
		}
	}
	else if (param == Pitch)
	{	
		int pitch = -1;
		for (; i < size; i++)
		{
			if (editType == SubtleEdit)
			{
				pitch = sequencer->step[hexUIToStep(editStack.notestack[i])].pitch + (((rand() % 0xffff) > 0x8000) ? subtleInterval : -subtleInterval);
				
				int octave = sequencer->step[hexUIToStep(editStack.notestack[i])].octave;
				if (pitch < 0)
				{
					octave -= 1;
					
					if (octave < 0) octave = 0;
					
					pitch+=12;
				}
				else if (pitch >= 12)
				{
					octave += 1;
					
					if (octave > 7) octave = 7;
					
					pitch-=12;
				}
				
				sequencer->step[hexUIToStep(editStack.notestack[i])].octave = octave;
				
				pitch = pitch % 12;
			}
			
			
			value =	(editType == RandomEdit) ? (((float) rand() / RAND_MAX) * 12) :
					(editType == SubtleEdit) ? pitch :
					value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].pitch = value;
			
			setParameterForEditStackSteps(inst, Note, 1);
		}
	}
	else if (param == Fine)	
	{		
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].fine + ((rand() % 0xffff) > 0x8000) ? SUBTLE_CV_AMT : -SUBTLE_CV_AMT) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].fine = value;
		}
	}
	else if (param == Octave)	
	{	
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (((float) rand() / RAND_MAX) * 8) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].octave + ((rand() % 0xffff) > 0x8000) ? 1 : -1) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].octave = value;
		}
	}
	else if (param == Note)		
	{	
		for (; i < size; i++)
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].note =	value; 
		}
	}
	else if (param == PitchGlide) 
	{ 
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].pglide + ((rand() % 0xffff) > 0x8000) ? 20 : -20) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].pglide = value;
		}
	}
	else if (param == CVGlide)	
	{	
		for (; i < size; i++)
		{
			value =	(editType == RandomEdit) ? (rand() / TWELVE_BIT_DIV) :
			(editType == SubtleEdit) ? (sequencer->step[hexUIToStep(editStack.notestack[i])].cvglide + ((rand() % 0xffff) > 0x8000) ? 20 : -20) :
			value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].cvglide = value;
		}
	}
	else if (param == On1)		
	{	
		for (; i < size; i++)
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelOne] =	value;
		}
	}
	else if (param == On2)		
	{	
		for (; i < size; i++)
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelTwo] =	value;
		}
	}
	else if (param == On3)			
	{
		for (; i < size; i++)
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelThree] =	value;
		}
	}
	else if (param == On4)			
	{
		for (; i < size; i++)
		{
			value = (editType == RandomEdit || editType == SubtleEdit) ? (((rand() % 0xffff) > 0x8000) ? 1 : 0) : value;
			
			sequencer->step[hexUIToStep(editStack.notestack[i])].on[PanelFour] =	value;
		}
	}
	else
	{
		
	}
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

		if (theStep->octave > 0)	theStep->octave -= 1;
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

uint8_t stepToHexUI(MantaInstrument inst, int step)
{
	step = (step < 0) ? 0 : step;
	
	if (full_vs_split == SplitMode)
	{
		if (step >= 16)
		{
			step = 15;
		}
		
		if (inst == InstrumentTwo)
		{
			step += 16;
		}
		
	}
	else
	{
		 if (step >= 32)
		{
			step = 31;
		}
	}
	
	return step;
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
void memoryInternalReadSequencer(int whichSeq, int whichhex, uint8_t* buffer)
{
	for (int i = 0; i < NUM_BYTES_PER_SEQUENCER; i++)
	{
		buffer[i] = memoryInternalCompositionBuffer[whichSeq][(whichhex*NUM_BYTES_PER_SEQUENCER) + i];
	}
}

void memoryInternalWriteSequencer(int whichSeq, int whichhex, uint8_t* buffer)
{
	for (int i = 0; i < NUM_BYTES_PER_SEQUENCER; i++)
	{
		memoryInternalCompositionBuffer[whichSeq][(whichhex*NUM_BYTES_PER_SEQUENCER) + i] = buffer[i];
	}
}

void memoryInternalCopySequencer(int sourceSeq, int sourceComp, int destSeq, int destComp)
{
	for (int i = 0; i < NUM_BYTES_PER_SEQUENCER; i++)
	{
		memoryInternalCompositionBuffer[destSeq][(destComp*NUM_BYTES_PER_SEQUENCER) + i] = 
		memoryInternalCompositionBuffer[sourceSeq][(sourceComp*NUM_BYTES_PER_SEQUENCER) + i];
	}
	
}

void indicateTransposition(int number)
{
	Write7Seg(abs(number));
	if (number < 0)
	{
		LED_On(PRESET_SAVE_LED); // this will double as the negative indicator for transpose
	}
	else
	{
		LED_Off(PRESET_SAVE_LED); // this will double as the negative indicator for transpose
	}
	transpose_indication_active = 1;
}
