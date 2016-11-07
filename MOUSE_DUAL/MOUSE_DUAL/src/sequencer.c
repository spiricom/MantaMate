/*
 * sequencer.c
 *
 * Created: 11/7/2016 9:53:57 AM
 *  Author: Jeff Snyder
 */ 

#include "sequencer.h"

uint8_t pattern_col_down[32] =			{0,8,16,24,  1,9,17,25,  2,10,18,26,  3,11,19,27,  4,12,20,28,  5,13,21,29,  6,14,22,30, 7,15,23,31};
uint8_t pattern_col_up[32] =	{24,16,8,0,  25,17,9,1,   26,18,10,2,   27,19,11,3,  28,20,12,4,  29,21,13,5,  30,22,14,6, 31,23,15,7};
uint8_t pattern_row_reverse[32] =	{7,6,5,4,3,2,1,0,15,14,13,12,11,10,9,8,23,22,21,20,19,18,17,16,31,30,29,28,27,26,25,24};

uint8_t pattern_diag[32] =			{0, 1,8,16, 2,9,17,24, 3,10,18,25, 4,11,19,26, 5,12,20,27, 6,13,21,28, 7,14,22,29, 15,23,30, 31};
uint8_t pattern_diag_reverse[32] =	{0, 16,8,1, 24,17,9,2, 25,18,10,3, 26,19,11,4, 27,20,12,5, 28,21,13,6, 29,22,14,7, 30,23,15, 31};

void tSequencer32SetPattern(tSequencer32 *seq, SequencerPatternType pat)
{
	seq->pattern = pat;
}

uint16_t tSequencerSet(tSequencer32 *seq, uint8_t step, StepParameterType paramType, uint16_t value)
{
	uint16_t val = 0;
	StepParameterType param = paramType;
	if (param == Toggled)
		seq->step[step].toggled = value;
	else if (param == Note)
		seq->step[step].note = value;
	else if (param == Hexagon)
		val = seq->step[step].hexagon;
	else if (param == Pitch)
		seq->step[step].pitch = value;
	else if (param == Octave)
		seq->step[step].octave = value;
	else if (param == CV1)
		seq->step[step].cv1 = value;
	else if (param == CV2)
		seq->step[step].cv2 = value;
	else if (param == CV3)
		seq->step[step].cv3 = value;
	else if (param == CV4)
		seq->step[step].cv4 = value;
	else if (param == Length) 
		seq->step[step].length = value;
	else 
		; // Other

	return val;
}

uint16_t tSequencerGet(tSequencer32 *seq, uint8_t step, StepParameterType paramType)
{
	uint16_t val = 0;
	StepParameterType param = paramType;
	
	if (param == Toggled)
		val = seq->step[step].toggled;
	else if (param == Note)
		val = seq->step[step].note;
	else if (param == Hexagon)
		val = seq->step[step].hexagon;
	else if (param == Pitch)
		val = seq->step[step].pitch;
	else if (param == Octave)
		val = seq->step[step].octave;
	else if (param == CV1)
		val = seq->step[step].cv1;
	else if (param == CV2)
		val = seq->step[step].cv2;
	else if (param == CV3)
		val = seq->step[step].cv3;
	else if (param == CV4)
		val = seq->step[step].cv4;
	else if (param == Length)
		val = seq->step[step].length;
	else
		; // Other
		
	return val;	
}

void tSequencer32Next(tSequencer32 *seq)
{
	int step = -1;
	int plusOrMinus;
	SequencerPatternType pat = seq->pattern;
	seq->stepGo = 1;
	
	while (seq->notestack.num > 0)
	{
		if (++seq->phasor >= 32) seq->phasor = 0;
		
		uint8_t stepPhasor = seq->phasor;
		
		if (pat == LeftRightRowDown)
			step = stepPhasor; 
		else if (pat == LeftRightRowUp)
			step = pattern_row_reverse[31 - stepPhasor];
		else if (pat == RightLeftRowDown)
			step = 31 - stepPhasor;
		else if (pat == RightLeftRowUp)
			step = pattern_row_reverse[stepPhasor];
		else if (pat == LeftRightColDown)
			step = pattern_col_down[stepPhasor];
		else if (pat == LeftRightColUp)
			step = pattern_col_up[stepPhasor];
		else if (pat == RightLeftColDown)
			step = pattern_col_down[31- stepPhasor];
		else if (pat == RightLeftColUp)
			step = pattern_col_up[31 - stepPhasor];
		else if (pat == LeftRightDiagDown)
			step = pattern_diag_reverse[stepPhasor];
		else if (pat == LeftRightDiagUp)
			step = pattern_diag[stepPhasor];
		else if (pat == RightLeftDiagDown)
			step = pattern_diag_reverse[31 - stepPhasor];
		else if (pat == RightLeftDiagUp)
			step = pattern_diag[31 - stepPhasor];
		else if (pat == RandomWalk)
		{
			plusOrMinus = rand();
			if (plusOrMinus > 0xffff)
			{
				step = (stepPhasor + (plusOrMinus-0xffff)) % 32;
			}
			else
			{
				step = (stepPhasor - plusOrMinus) % 32;
			}
		}
		else if (pat == OrderTouch)
			step = seq->notestack.next(&seq->notestack);
		else
			;//Other

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

int tSequencer32GetNumNotes(tSequencer32 *seq)
{
	return seq->notestack.num;
}

int tSequencer32Init(tSequencer32 *seq) 
{
	seq->currentStep = 0;
	seq->prevStep = 0;
	seq->lengthCounter = 0;
	seq->phasor = 0;
	seq->pattern = LeftRightRowDown;
	seq->next = &tSequencer32Next;
	seq->setPattern = &tSequencer32SetPattern;
	seq->get = &tSequencerGet;
	seq->set = &tSequencerSet;
	seq->getNumNotes = &tSequencer32GetNumNotes;
	
	for (int i = 0; i < 32; i++)
	{
		seq->step[i].cv1 = 0;  // cv1 zero
		seq->step[i].cv2 = 0;  // cv2 zero
		seq->step[i].pitch = 0;  // keyboard pitch zero
		seq->step[i].note = 1;  // note, not rest
		seq->step[i].toggled = 0;  // not toggled on
		seq->step[i].cv3 = 0;  // cv3 zero
		seq->step[i].cv4 = 0;  // cv4 zero
		seq->step[i].octave = 3;  // octave
		seq->step[i].length = 1;  // step_length = 1
		seq->step[i].hexagon = 32;  // hexagon number in keyboard range
	}
	
	tNoteStack32Init(&seq->notestack);
	
	
	
	return 0;
}

