/*
 * sequencer.c
 *
 * Created: 11/7/2016 9:53:57 AM
 *  Author: Jeff Snyder
 */ 

#include "sequencer.h"

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


void tSequencerSetPattern(tSequencer *seq, SequencerPatternType pat)
{
	seq->pattern = pat;
}

uint16_t tSequencerSet(tSequencer *seq, uint8_t step, StepParameterType paramType, uint16_t value)
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

	return val;
}

uint16_t tSequencerGet(tSequencer *seq, uint8_t step, StepParameterType paramType)
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


int tSequencerGetHexFromStep(tSequencer *seq, uint8_t in)
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
		hex = seq->notestack.next(&seq->notestack);
	else
		;//Other
		
	return hex;
}

void tSequencerNext(tSequencer *seq)
{
	
	
	seq->stepGo = 1;
	int step = -1;
	
	while (seq->notestack.size > 0)
	{
		if (++seq->phasor >= seq->maxLength) seq->phasor = 0;
		
		step = tSequencerGetHexFromStep(seq, seq->phasor);
		
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

int tSequencerGetNumNotes(tSequencer *seq)
{
	return seq->notestack.size;
}

int tSequencerSetMaxLength(tSequencer *seq, uint8_t maxLength)
{	
	seq->maxLength = maxLength;
	seq->notestack.setCapacity(&seq->notestack,maxLength);
}

void tSequencerSetOctave(tSequencer *seq, int8_t octave)
{
	seq->octave = octave;
}

int tSequencerGetOctave(tSequencer *seq, int8_t octave)
{
	return seq->octave;
}


void tSequencerDownOctave(tSequencer *seq)
{
	if (--seq->octave < 0)
	{
		seq->octave = 0;
	}
}

void tSequencerUpOctave(tSequencer *seq)
{
	if (++seq->octave > 7)
	{
		seq->octave = 7;
	}
}

int tSequencerStepToggle(tSequencer *seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	foundOne = seq->notestack.remove(&seq->notestack,step);
	
	if (!foundOne)
	{
		seq->step[step].toggled = 1;
		seq->notestack.add(&seq->notestack,step);
	}
	else
	{
		seq->step[step].toggled = 0;
	}

	return !foundOne;
}

int tSequencerStepAdd(tSequencer *seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	foundOne = seq->notestack.contains(&seq->notestack, step);
	
	if (foundOne < 0)
	{
		seq->step[step].toggled = 1;
		seq->notestack.add(&seq->notestack,step);
	}

	return !foundOne;
}

int tSequencerStepRemove(tSequencer *seq, uint8_t step)
{
	uint8_t foundOne = 0;
	
	seq->step[step].toggled = 0;
	foundOne = seq->notestack.remove(&seq->notestack, step);

	return foundOne;
}

void tSequencerClearSteps(tSequencer *seq)
{
	for (int i = 0; i < MAX_STEPS; i++)
	{
		seq->step[i].toggled = 0;
		seq->notestack.remove(&seq->notestack, i);
	}
}

int tSequencerInit(tSequencer *seq, uint8_t maxLength) 
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
	
	seq->clearSteps = &tSequencerClearSteps;
	seq->toggleStep = &tSequencerStepToggle;
	seq->addStep = &tSequencerStepAdd;
	seq->removeStep = &tSequencerStepRemove;
	seq->next = &tSequencerNext;
	seq->setPattern = &tSequencerSetPattern;
	seq->get = &tSequencerGet;
	seq->set = &tSequencerSet;
	seq->setOctave = &tSequencerSetOctave;
	seq->getOctave = &tSequencerGetOctave;
	seq->upOctave = &tSequencerUpOctave;
	seq->downOctave = &tSequencerDownOctave;
	seq->getNumNotes = &tSequencerGetNumNotes;
	seq->setMaxLength = &tSequencerSetMaxLength;
	seq->getHexFromStep = &tSequencerGetHexFromStep;
	
	for (int i = 0; i < MAX_STEPS; i++)
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
		seq->step[i].fine = 2048; // 2948 is no fine tune offset. 0-2047 is negative, 2048-4095 is positive
		seq->step[i].octave = 3;  // octave
		seq->step[i].kbdhex = MAX_STEPS + 0;  // hexagon number in keyboard range
		
		// Trigger only
		for (int j = 0; j < 4; j++)
		{
			seq->step[i].on[j] = 0;
		}
	}
	
	tNoteStackInit(&seq->notestack, maxLength);

	return 0;
}



