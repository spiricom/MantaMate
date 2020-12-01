/*
 * sequencer.c
 *
 * Created: 11/7/2016 9:53:57 AM
 *  Author: Jeff Snyder
 */ 

#include "sequencer.h"
#include "memory_spi.h"


uint8_t pattern_col_down[MAX_STEPS] =				{0,8,16,24,  1,9,17,25,  2,10,18,26,  3,11,19,27,  4,12,20,28,  5,13,21,29,  6,14,22,30, 7,15,23,31};
	
uint8_t pattern_col_up[MAX_STEPS] =					{24,16,8,0,  25,17,9,1,   26,18,10,2,   27,19,11,3,  28,20,12,4,  29,21,13,5,  30,22,14,6, 31,23,15,7};
uint8_t pattern_row_reverse[MAX_STEPS] =			{7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24};
	
uint8_t pattern_caterpillar[MAX_STEPS] =			{0,8,1,9,2,10,3,11,4,12,5,13,6,14,7,15,16,24,17,25,18,26,19,27,20,28,21,29,22,30,23,31};

uint8_t pattern_diag[MAX_STEPS] =			{0, 1,8,16, 2,9,17,24, 3,10,18,25, 4,11,19,26, 5,12,20,27, 6,13,21,28, 7,14,22,29, 15,23,30, 31};
uint8_t pattern_diag_reverse[MAX_STEPS] =	{0, 16,8,1, 24,17,9,2, 25,18,10,3, 26,19,11,4, 27,20,12,5, 28,21,13,6, 29,22,14,7, 30,23,15, 31};
	
	
uint8_t patterns[5][MAX_STEPS] = {
	{0,8,16,24,  1,9,17,25,  2,10,18,26,  3,11,19,27,  4,12,20,28,  5,13,21,29,  6,14,22,30, 7,15,23,31}, 
	{24,16,8,0,  25,17,9,1,   26,18,10,2,   27,19,11,3,  28,20,12,4,  29,21,13,5,  30,22,14,6, 31,23,15,7}, 
	{7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24}, 
	{0, 1,8,16, 2,9,17,24, 3,10,18,25, 4,11,19,26, 5,12,20,27, 6,13,21,28, 7,14,22,29, 15,23,30, 31}, 
	{0, 16,8,1, 24,17,9,2, 25,18,10,3, 26,19,11,4, 27,20,12,5, 28,21,13,6, 29,22,14,7, 30,23,15, 31}
		};

int numBytesPerHex = 19; //change this if the number of bytes per hex in an "encoded" serialized sequence changes.

void tSequencer_setPattern(tSequencer* const seq, SequencerPatternType pat)
{
	seq->pattern = pat;
}

void tSequencer_setParameterValue(tSequencer* const seq, uint8_t step, StepParameterType paramType, uint16_t value)
{
	StepParameterType param = paramType;
	
	if (param == Toggled)				seq->step[step].toggled = value;
	else if (param == Length)			seq->step[step].length = value;
	else if (param == CV1)				seq->step[step].cv1 = value;
	else if (param == CV2)				seq->step[step].cv2 = value;
	else if (param == CV3)				seq->step[step].cv3 = value;
	else if (param == CV4)				seq->step[step].cv4 = value;
	else if (param == Pitch)			seq->step[step].pitch = value;
	else if (param == Fine)				seq->step[step].fine = value;
	else if (param == Octave)			seq->step[step].octave = value;
	else if (param == Note)				seq->step[step].note = value;
	else if (param == PitchGlide)		seq->step[step].pglide = value;
	else if (param == CVGlide)			seq->step[step].cvglide = value;
	else if (param == On1)				seq->step[step].on[PanelOne] = value;
	else if (param == On2)				seq->step[step].on[PanelTwo] = value;
	else if (param == On3)				seq->step[step].on[PanelThree] = value;
	else if (param == On4)				seq->step[step].on[PanelFour] = value;
	else; // Other
}

uint16_t tSequencer_getParameterValue(tSequencer* const seq, uint8_t step, StepParameterType paramType)
{
	uint16_t val = 0;
	StepParameterType param = paramType;
	
	if (param == Toggled)			val = seq->step[step].toggled;
	else if (param == Length)		val = seq->step[step].length;
	else if (param == CV1)			val = seq->step[step].cv1;
	else if (param == CV2)			val = seq->step[step].cv2;
	else if (param == CV3)			val = seq->step[step].cv3;
	else if (param == CV4)			val = seq->step[step].cv4;
	else if (param == Pitch)		val = seq->step[step].pitch;
	else if (param == Fine)			val = seq->step[step].fine;
	else if (param == Octave)		val = seq->step[step].octave;
	else if (param == Note)			val = seq->step[step].note;
	else if (param == PitchGlide)	val = seq->step[step].pglide;
	else if (param == CVGlide)		val = seq->step[step].cvglide;
	else if (param == On1)			val = seq->step[step].on[PanelOne];
	else if (param == On2)			val = seq->step[step].on[PanelTwo];
	else if (param == On3)			val = seq->step[step].on[PanelThree];
	else if (param == On4)			val = seq->step[step].on[PanelFour];
	else;
		
	return val;	
}



static uint8_t plusOrMinus;

int tSequencer_getHexFromStep(tSequencer* const seq, uint8_t in)
{
	SequencerPatternType pat = seq->pattern;
	int hex = -1;

	
	if (pat == LeftRightRowDown)
		hex = pattern_row_reverse[(seq->maxLength - 1) - in];
	else if (pat == LeftRightRowUp)
		hex = in;
	else if (pat == RightLeftRowDown)
		hex = (seq->maxLength - 1) - in;
	else if (pat == RightLeftRowUp)
		hex = pattern_row_reverse[in];
	else if (pat == LeftRightColDown)
		hex = pattern_col_up[in];
	else if (pat == LeftRightColUp)
		hex = pattern_col_down[in];
	else if (pat == RightLeftColDown)
		hex = pattern_col_down[(seq->maxLength - 1)- in];
	else if (pat == RightLeftColUp)
		hex = pattern_col_up[(seq->maxLength - 1) - in];
	else if (pat == LeftRightDiagDown)
		hex = pattern_diag_reverse[in];
	else if (pat == LeftRightDiagUp)
		hex = pattern_diag[in];
	else if (pat == RightLeftDiagDown)
		hex = pattern_diag_reverse[(seq->maxLength - 1) - in];
	else if (pat == RightLeftDiagUp)
		hex = pattern_diag[(seq->maxLength - 1) - in];
	else if (pat == RandomPattern) //needs fixing - not sure I understand what you're up to well enough to fix...   as far as I understand it, the "in" input is the seq phasor, which can up to up to numbers beyond the current hex step number (i.e. up to 32). If that's true, how can we grab the current hex value so that we can return the properly incremented version? I'm confused...
	{
		if (seq->reverse)
		{
			plusOrMinus = (uint8_t)((rand() >> 15) & 1); //coin flip   right shifting by 15 to avoid cyclic pattern in low 12 bits or so
			hex = seq->currentStep;
			if (plusOrMinus > 0)
			{
				//go up
				while(1)
				{
					hex = (hex + 1) % seq->maxLength;
					if (seq->step[hex].toggled == 1)
					{
						break;
					}
				}
			}
			else
			{
				//go down
				while(1)
				{
					if (hex > 0)
					{
						hex = (hex - 1) % seq->maxLength;
						if (seq->step[hex].toggled == 1)
						{
							break;
						}
					}
					else
					{
						hex = seq->maxLength-1;
						if (seq->step[hex].toggled == 1)
						{
							break;
						}
					}

				}
			}
		}
		else
		{
			hex = (rand() >> 8) % seq->maxLength;
		}
		
	}
	else if (pat == Caterpillar)
	{
		hex = pattern_caterpillar[in];
	}
	else if (pat == OrderTouch)
		hex = seq->notestack.notestack[in];
	else
		;//Other
		
	return hex;
}

// utility
static int getIndexInPattern(uint8_t* pattern, uint8_t value)
{
	// All patterns are MAX_STEPS length
	for (int i = 0; i < MAX_STEPS; i++)	if (pattern[i] == value) return i;
	return -1;
}

int tSequencer_getStepFromHex(tSequencer* const seq, uint8_t hex)
{
	SequencerPatternType pat = seq->pattern;
	int step = 0;
	// WORKING: 1, 2, 7
	// NOT WORKING: 3, 4, 5, 6 

	if (pat == LeftRightRowDown)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_row_reverse, hex);
	else if (pat == LeftRightRowUp)
		step = hex;
	else if (pat == LeftRightDiagDown)
		step = getIndexInPattern(pattern_diag_reverse, hex);
	else if (pat == LeftRightDiagUp)
		step = getIndexInPattern(pattern_diag, hex);
	else if (pat == LeftRightColDown)
		step = getIndexInPattern(pattern_col_up, hex);
	else if (pat == RightLeftColUp)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_col_up, hex);
	else if (pat == OrderTouch)
	{
			// TODO: Decide what to do for Order. Just doing LeftRightRowDown for now
			step = (seq->maxLength - 1) - getIndexInPattern(pattern_row_reverse, hex);
	}
	else if (pat == LeftRightColUp)
		step = getIndexInPattern(pattern_col_down, hex);
	else if (pat == RightLeftRowDown)
		step = (seq->maxLength - 1) - hex;
	else if (pat == RightLeftRowUp)
		step = getIndexInPattern(pattern_row_reverse, hex);
	else if (pat == RightLeftColDown)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_col_down, hex);
	else if (pat == RightLeftDiagDown)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_diag_reverse, hex);
	else if (pat == Caterpillar)
		step = getIndexInPattern(pattern_caterpillar, hex);
	else if (pat == RightLeftDiagUp)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_diag, hex);
	
	
	return step;
	
}

void tSequencer_next(tSequencer* const seq)
{
	seq->stepGo = 1;
	int hex = -1;
	
	while (seq->notestack.size > 0)
	{
		if (!seq->reverse)
		{
			if (++seq->phasor >= seq->maxLength) seq->phasor = 0;
		}
		else
		{
			if (--seq->phasor < 0) seq->phasor = seq->maxLength-1;
		}
		
		hex = tSequencer_getHexFromStep(seq, seq->phasor);
		
		if (seq->step[hex].toggled == 1)
		{
			break;
		}
	}
	
	if (hex < 0) 
	{
		seq->stepGo = 0;
		seq->currentStep = 0;
	}
	else
	{
		seq->prevStep = seq->currentStep;
		seq->currentStep = hex;
	}
}

int tSequencer_getNumNotes(tSequencer* const seq)
{
	return seq->notestack.size;
}

void tSequencer_setMaxLength(tSequencer* const seq, uint8_t maxLength)
{	
	seq->maxLength = maxLength;
	tNoteStack_setCapacity(&seq->notestack,maxLength);
}

void tSequencer_setOctave(tSequencer* const seq, int8_t octave)
{
	seq->octave = octave;
}

int tSequencer_getOctave(tSequencer* const seq)
{
	return seq->octave;
}


void tSequencer_downOctave(tSequencer* const seq)
{
	if (--seq->octave < 0)
	{
		seq->octave = 0;
	}
}

void tSequencer_upOctave(tSequencer* const seq)
{
	if (++seq->octave > 7)
	{
		seq->octave = 7;
	}
}

int tSequencer_toggleStep(tSequencer* const seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	foundOne = tNoteStack_remove(&seq->notestack,step);
	
	if (!foundOne)
	{
		seq->step[step].toggled = 1;
		tNoteStack_add(&seq->notestack,step);
	}
	else
	{
		seq->step[step].toggled = 0;
	}

	return !foundOne;
}

int tSequencer_addStep(tSequencer* const seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	foundOne = tNoteStack_contains(&seq->notestack, step);
	
	if (foundOne < 0)
	{
		seq->step[step].toggled = 1;
		tNoteStack_add(&seq->notestack,step);
	}

	return !foundOne;
}

int tSequencer_removeStep(tSequencer* const seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	seq->step[step].toggled = 0;
	foundOne = tNoteStack_remove(&seq->notestack, step);

	return foundOne;
}

void tSequencer_clearSteps(tSequencer* const seq)
{
	for (int i = 0; i < MAX_STEPS; i++)
	{
		seq->step[i].toggled = 0;
		tNoteStack_remove(&seq->notestack, i);
	}
}

void tSequencer_randomizePitch(tSequencer* const seq)
{
	
}

void tSequencer_randomizeCV(tSequencer* const seq)
{
	
}

void tSequencer_randomizeStepLength(tSequencer* const seq)
{
	
}

void tSequencer_randomizeGlide(tSequencer* const seq)
{
	
}

void tSequencer_randomizeToggled(tSequencer* const seq)
{
	
}

void tSequencer_randomizeTrigger(tSequencer* const seq)
{
	
}

void tSequencer_randomizeAll(tSequencer* const seq)
{	
	seq->transpose = 0;
	seq->octave = (rand() >> 15) % 8;
		
	for (int i = 0; i < 4; i++) seq->mute[i] = FALSE;
	tNoteStack_init(&seq->notestack, 32);
	
	for (int i = 0; i < 32; i++)
	{
		// Pitch and Trigger parameters
		BOOL tempToggle = ((rand() >> 15) & 1);
		if (tempToggle)
		{
			tNoteStack_add(&seq->notestack, i);
		}
		seq->step[i].toggled = tempToggle;
		
		seq->step[i].length = 1;  // step_length = 1
		seq->step[i].cv1 = ((rand() >> 15) % 4096); 
		seq->step[i].cv2 = ((rand() >> 15) % 4096); 	
		seq->step[i].cv3 = ((rand() >> 15) % 4096); 
		seq->step[i].cv4 = ((rand() >> 15) % 4096); 
		if (coinToss(80))
		{
			seq->step[i].note = 1; 
		}
		else
		{
			seq->step[i].note = 0; 
		}
		seq->step[i].pitch =  ((rand() >> 15) % 12); 
		seq->step[i].fine = ((rand() >> 15) % 4096); 
		seq->step[i].octave = ((rand() >> 15) % 8); 
		
		if (coinToss(8))
		{
			seq->step[i].pglide = ((rand() >> 15) & 512);
		}
		else
		{
			seq->step[i].pglide = 0;
		}
		if (coinToss(8))
		{
			seq->step[i].cvglide = ((rand() >> 15) & 1) * ((rand() >> 15) & 512);
		}
		else
		{
			seq->step[i].pglide = 7;
		}
		for (int j = 0; j < 4; j++)
		{
			seq->step[i].on[j] = ((rand() >> 15) & 1);
		}
	}
}



void tSequencer_deviate(tSequencer* const seq)
{	
	
	int CVProb = 8;
		
	for (int i = 0; i < 32; i++)
	{
	
		if (coinToss(5))
		{
			if (coinToss(50))
			{
				seq->step[i].length = (seq->step[i].length+1) % 9;  // step_length = 1
			}
			else
			{
				if (seq->step[i].length > 1)
				{
					seq->step[i].length--;
				}
			}
		}
		
		if (coinToss(CVProb))
		{
			if (coinToss(50))
			{
				seq->step[i].cv1 = (seq->step[i].cv1 + ((rand() >> 15) % 1024)) % 4096; 
			}
			else
			{
				seq->step[i].cv1  = (seq->step[i].cv1 - (rand() >> 15) % 1024) % 4096;
			}
			
		}
		if (coinToss(CVProb))
		{
			if (coinToss(50))
			{
				seq->step[i].cv2 = (seq->step[i].cv2 + ((rand() >> 15) % 1024)) % 4096; 
			}
			else
			{
				seq->step[i].cv2 = (seq->step[i].cv2 - (rand() >> 15) % 1024) % 4096;
			}
			
		}
		if (coinToss(CVProb))
		{
			if (coinToss(50))
			{
				seq->step[i].cv3 = (seq->step[i].cv3 + ((rand() >> 15) % 1024)) % 4096; 
			}
			else
			{
				seq->step[i].cv3 = (seq->step[i].cv3 - (rand() >> 15) % 1024) % 4096;
			}
			
		}
		if (coinToss(CVProb))
		{
			if (coinToss(50))
			{
				seq->step[i].cv3 = (seq->step[i].cv3 + ((rand() >> 15) % 1024)) % 4096; 
			}
			else
			{
				seq->step[i].cv3 = (seq->step[i].cv3 - (rand() >> 15) % 1024) % 4096;
			}
			
		}
		if (coinToss(8))
		{
			seq->step[i].note = ((rand() >> 15) & 1); 
		}
		if (coinToss(10))
		{
			seq->step[i].pitch =  ((rand() >> 15) % 12); 
		}
		if (coinToss(5))
		{
			seq->step[i].fine = ((rand() >> 15) % 4096); 
		}
		if (coinToss(10))
		{
			seq->step[i].octave = ((rand() >> 15) % 8);
		}
		if (coinToss(5))
		{
			if (coinToss(20))
			{
				seq->step[i].pglide = ((rand() >> 15) & 512);
			}
			else
			{
				seq->step[i].pglide = 0;
			}
		}
		if (coinToss(5))
		{
			if (coinToss(20))
			{
				seq->step[i].cvglide = ((rand() >> 15) & 512);
			}
			else
			{
				seq->step[i].cvglide = 7;
			}
		}
		for (int j = 0; j < 4; j++)
		{
			if (coinToss(5))
			{
				seq->step[i].on[j] = !(seq->step[i].on[j]);
			}
		}
	}
	
}

int tSequencer_clear(tSequencer* const seq)
{
	seq->reverse = FALSE;
	seq->lastTouch = 0;
	seq->transpose = 0;
	seq->currentStep = 0;
	seq->prevStep = 0;
	seq->lengthCounter = 0;
	seq->phasor = 0;
	seq->pattern = LeftRightRowUp;
	seq->octave = 3;
	seq->playMode = ToggleMode;
	
	for (int i = 0; i < 4; i++) seq->mute[i] = FALSE;

	for (int i = 0; i < 32; i++)
	{
		// Pitch and Trigger parameters
		seq->step[i].toggled = 0;  // not toggled on
		seq->step[i].length = 1;  // step_length = 1
		seq->step[i].cv1 = 0;  // cv1 zero
		seq->step[i].cv2 = 0;  // cv2 zero
		

		seq->step[i].cv3 = 0;  // cv3 zero
		seq->step[i].cv4 = 0;  // cv4 zero
		seq->step[i].note = 1;  // note, not rest
		seq->step[i].pitch = 0;  // keyboard pitch zero
		seq->step[i].fine = 2048; // 2048 is no fine tune offset. 0-2047 is negative, 2048-4095 is positive
		seq->step[i].octave = 3;  // octave
		seq->step[i].pglide = 1;
		seq->step[i].cvglide = 5;


		for (int j = 0; j < 4; j++)
		{
			seq->step[i].on[j] = 0;
		}
	}
	
	tNoteStack_init(&seq->notestack, 32);


	return 0;
}

int tSequencer_init(tSequencer* const seq, GlobalOptionType type, uint8_t maxLength) 
{
	if (maxLength < 1)
	{
		seq->maxLength = 1;
	}
	else if (maxLength <= 32)
	{
		seq->maxLength = maxLength;
	}
	else
	{
		seq->maxLength = 32;
	}
	
	seq->reverse = FALSE;
	seq->lastTouch = 0;
	seq->transpose = 0;
	seq->currentStep = 0;
	seq->prevStep = 0;
	seq->lengthCounter = 0;
	seq->phasor = 0;
	seq->pattern = LeftRightRowUp;
	seq->octave = 3;
	seq->pitchOrTrigger = type;
	seq->playMode = ToggleMode;
	
	for (int i = 0; i < 4; i++) seq->mute[i] = FALSE;
	
	for (int i = 0; i < 32; i++)
	{
		// Pitch and Trigger parameters
		seq->step[i].toggled = 0;  // not toggled on
		seq->step[i].length = 1;  // step_length = 1
		seq->step[i].cv1 = 0;  // cv1 zero
		seq->step[i].cv2 = 0;  // cv2 zero
		
		// Pitch only
		seq->step[i].cv3 = 0;  // cv3 zero
		seq->step[i].cv4 = 0;  // cv4 zero
		seq->step[i].note = 1;  // note, not rest
		seq->step[i].pitch = 0;  // keyboard pitch zero
		seq->step[i].fine = 2048; // 2048 is no fine tune offset. 0-2047 is negative, 2048-4095 is positive
		seq->step[i].octave = 3;  // octave
		seq->step[i].pglide = 5;
		seq->step[i].cvglide = 5;
		
		// Trigger only
		for (int j = 0; j < 4; j++)
		{
			seq->step[i].on[j] = 0;
		}
	}
	
	tNoteStack_init(&seq->notestack, maxLength);


	return 0;
}





/*
these are packed into a single byte since they are 1-bit values
[Toggled,
note,
On1,
On2,
On3,
On4,
]
//then these come after that byte
Length,
CV1,
CV2,
CV3,
CV4,
Pitch,
Fine,
Octave,
PitchGlide,
CVGlide
*/

void        tSequencer_encode(tSequencer* const seq, uint8_t* sBuffer)
{
	sBuffer[SeqOctave] = seq->octave;
	sBuffer[SeqPitchOrTrigger] = seq->pitchOrTrigger;
	sBuffer[SeqTransposeHigh] = seq->transpose >> 8;
	sBuffer[SeqTransposeLow] = seq->transpose & 0xff;
	sBuffer[SeqPattern] = seq->pattern;
	sBuffer[SeqPlayMode] = seq->playMode;
	for (int hex = 0; hex < 32; hex++)
	{
		int offset = SeqSteps + hex * numBytesPerHex;
		
		uint8_t tempPackedData = (seq->step[hex].toggled);
		tempPackedData += (seq->step[hex].note) << 1;
		tempPackedData += (seq->step[hex].on[0]) << 2;
		tempPackedData += (seq->step[hex].on[1]) << 3;
		tempPackedData += (seq->step[hex].on[2]) << 4;
		tempPackedData += (seq->step[hex].on[3]) << 5;
		
		sBuffer[offset+0] = tempPackedData;
		sBuffer[offset+1] = seq->step[hex].length;
		sBuffer[offset+2] = (seq->step[hex].cv1 >> 8) & 255;
		sBuffer[offset+3] = seq->step[hex].cv1 & 255;
		sBuffer[offset+4] = (seq->step[hex].cv2 >> 8) & 255;
		sBuffer[offset+5] = seq->step[hex].cv2 & 255;
		sBuffer[offset+6] = (seq->step[hex].cv3 >> 8) & 255;
		sBuffer[offset+7] = seq->step[hex].cv3 & 255;
		sBuffer[offset+8] = (seq->step[hex].cv4 >> 8) & 255;
		sBuffer[offset+9] = seq->step[hex].cv4 & 255;
		sBuffer[offset+10] = seq->step[hex].pitch;
		sBuffer[offset+11] = (seq->step[hex].fine >> 8) & 255;
		sBuffer[offset+12] = seq->step[hex].fine & 255;
		sBuffer[offset+13] = seq->step[hex].octave;
		sBuffer[offset+14] = (seq->step[hex].pglide >> 8) & 255;
		sBuffer[offset+15] = seq->step[hex].pglide & 255;
		sBuffer[offset+16] = (seq->step[hex].cvglide >> 8) & 255;
		sBuffer[offset+17] = seq->step[hex].cvglide & 255;
	}
}

void	 tSequencer_decode(tSequencer* const seq, uint8_t* sBuffer)
{
	tNoteStack_clear(&seq->notestack);
	
	seq->octave = sBuffer[SeqOctave];
	seq->pitchOrTrigger = sBuffer[SeqPitchOrTrigger];
	seq->transpose = (sBuffer[SeqTransposeHigh] << 8) + sBuffer[SeqTransposeLow];
	seq->pattern = sBuffer[SeqPattern];
	seq->playMode = sBuffer[SeqPlayMode];
	for (int hex = 0; hex < 32; hex++)
	{
		int offset = SeqSteps + hex * numBytesPerHex;
		
		int toggled = sBuffer[offset+0] & 1;
		seq->step[hex].toggled = toggled;
		if (toggled) tNoteStack_add(&seq->notestack, hex);
		
		seq->step[hex].note = (sBuffer[offset+0] >> 1) & 1;
		seq->step[hex].on[0] = (sBuffer[offset+0] >> 2) & 1;
		seq->step[hex].on[1] = (sBuffer[offset+0] >> 3) & 1;
		seq->step[hex].on[2] = (sBuffer[offset+0] >> 4) & 1;
		seq->step[hex].on[3] = (sBuffer[offset+0] >> 5) & 1;
		
		seq->step[hex].length = sBuffer[offset+1];
		seq->step[hex].cv1 = (sBuffer[offset+2] << 8) + sBuffer[offset+3];//
		seq->step[hex].cv2 =(sBuffer[offset+4] << 8) + sBuffer[offset+5];//
		seq->step[hex].cv3 =(sBuffer[offset+6] << 8) + sBuffer[offset+7];//
		seq->step[hex].cv4 =(sBuffer[offset+8] << 8) + sBuffer[offset+9];//
		seq->step[hex].pitch = sBuffer[offset+10];
		seq->step[hex].fine = (sBuffer[offset+11] << 8) + sBuffer[offset+12];//
		seq->step[hex].octave = sBuffer[offset+13];
		seq->step[hex].pglide = (sBuffer[offset+14] << 8) + sBuffer[offset+15];//
		seq->step[hex].cvglide = (sBuffer[offset+16] << 8) + sBuffer[offset+17];//
	}
}

