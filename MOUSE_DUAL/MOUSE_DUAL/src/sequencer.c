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
	uint16_t val = 0;
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
	else if (param == KbdHex)			seq->step[step].kbdhex = value;
	else if (param == PitchGlide)		seq->step[step].pglide = value;
	else if (param ==CVGlide)			seq->step[step].cvglide = value;
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
	else if (param == KbdHex)		val = seq->step[step].kbdhex;
	else if (param == PitchGlide)	val = seq->step[step].pglide;
	else if (param == CVGlide)		val = seq->step[step].cvglide;
	else if (param == On1)			val = seq->step[step].on[PanelOne];
	else if (param == On2)			val = seq->step[step].on[PanelTwo];
	else if (param == On3)			val = seq->step[step].on[PanelThree];
	else if (param == On4)			val = seq->step[step].on[PanelFour];
	else;
		
	return val;	
}


/*
[ //these are packed into a single byte since they are 1-bit values
Toggled,
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
Note,
KbdHex,
PitchGlide,
CVGlide
*/
// we're using 16_bit ints even though we are only using the first 8 bits of them because using 8_bit ints didn't work for SPI sending functions for some reason... 
// would be nice to figure out why and reduce the size of these arrays, since they are wasting space on the chip -JS
void        tSequencer_encode(tSequencer* const seq, uint16_t* sBuffer)
{
	sBuffer[SeqOctave] = seq->octave;
	sBuffer[SeqMaxLength] = seq->maxLength;
	sBuffer[SeqPitchOrTrigger] = seq->pitchOrTrigger;
	
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
		sBuffer[offset+14] = seq->step[hex].kbdhex;
		sBuffer[offset+15] = (seq->step[hex].pglide >> 8) & 255;
		sBuffer[offset+16] = seq->step[hex].pglide & 255;
		sBuffer[offset+17] = (seq->step[hex].cvglide >> 8) & 255;
		sBuffer[offset+18] = seq->step[hex].cvglide & 255;
	}
}

uint16_t myglobaltest = 0;
void        tSequencer_decode(tSequencer* const seq, uint16_t* sBuffer)
{
	tNoteStack_clear(&seq->notestack);
	
	seq->octave = sBuffer[SeqOctave];
	seq->maxLength= sBuffer[SeqMaxLength];
	seq->pitchOrTrigger = sBuffer[SeqPitchOrTrigger];
	
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
		seq->step[hex].kbdhex = sBuffer[offset+14];
		seq->step[hex].pglide = (sBuffer[offset+15] << 8) + sBuffer[offset+16];//
		seq->step[hex].cvglide = (sBuffer[offset+17] << 8) + sBuffer[offset+18];//
		
	}
}


int tSequencer_getHexFromStep(tSequencer* const seq, uint8_t in)
{
	SequencerPatternType pat = seq->pattern;
	int hex = -1;
	int plusOrMinus;
	
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
	else if (pat == RandomWalk)
	{
		plusOrMinus = rand();
		if (plusOrMinus > 0xffff)
		{
			hex = (in + (plusOrMinus-0xffff)) % seq->maxLength;
		}
		else
		{
			hex = (in - plusOrMinus) % seq->maxLength;
		}
	}
	else if (pat == OrderTouch)
		hex = tNoteStack_next(&seq->notestack);
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
	int step = -1;
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
	else if (pat == RandomWalk) 
	{
			// TODO: Decide what to do for  Random. Just doing LeftRightRowDown for now
			step = (seq->maxLength - 1) - getIndexInPattern(pattern_row_reverse, hex);
	}
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
	else if (pat == RightLeftDiagUp)
		step = (seq->maxLength - 1) - getIndexInPattern(pattern_diag, hex);
	
	
	return step;
	
}

void tSequencer_next(tSequencer* const seq)
{
	seq->stepGo = 1;
	int step = -1;
	
	while (seq->notestack.size > 0)
	{
		if (++seq->phasor >= seq->maxLength) seq->phasor = 0;
		
		step = tSequencer_getHexFromStep(seq, seq->phasor);
		
		if (seq->step[step].toggled == 1)
		{
			break;
		}
	}
	
	if (step < 0) 
	{
		seq->stepGo = 0;
		seq->currentStep = 0;
	}
	else
	{
		seq->prevStep = seq->currentStep;
		seq->currentStep = step;
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
	
	seq->currentStep = 0;
	seq->prevStep = 0;
	seq->lengthCounter = 0;
	seq->phasor = 0;
	seq->pattern = LeftRightRowDown;
	seq->octave = 3;
	seq->pitchOrTrigger = type;
	
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
		seq->step[i].kbdhex = MAX_STEPS + 0;  // hexagon number in keyboard range
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






