/*
 * sequencer.h
 *
 * Created: 11/7/2016 9:54:07 AM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include "stdlib.h"

#include "notestack.h"

uint8_t pattern_col_down[32];
uint8_t pattern_col_up[32];
uint8_t pattern_row_reverse[32];

uint8_t pattern_diag[32];
uint8_t pattern_diag_reverse[32];

typedef enum SequencerPatternType {
	LeftRightRowDown,
	RightLeftRowDown,
	LeftRightRowUp,
	RightLeftRowUp,
	LeftRightDiagDown,
	LeftRightDiagUp,
	RightLeftDiagDown,
	RightLeftDiagUp,
	LeftRightColDown,
	LeftRightColUp,
	RightLeftColDown,
	RightLeftColUp,
	RandomWalk,
	OrderTouch,
	RecordTouch,
	SequencerPatternTypeNil,
}SequencerPatternType;

typedef enum StepParameterType {
	Hexagon,
	Toggled,
	CV1,
	CV2,
	CV3,
	CV4,
	Pitch,
	Note,
	Octave,
	Length
}StepParameterType;

typedef struct _tStep
{
	uint16_t hexagon;       //[9]
	uint16_t toggled;       //[4]

	uint16_t cv1;       //[0]
	uint16_t cv2;       //[1]
	uint16_t cv3;       //[5]
	uint16_t cv4;       //[6]

	uint16_t pitch;     //[2]
	uint16_t note;          //[3]
	uint16_t octave;    //[7]
	uint16_t length;    //[8]
		
} tStep;

typedef struct _tSequencer32
{

	tStep step[32];
	int phasor;
	int currentStep,prevStep;
	int stepGo;
	tNoteStack32 notestack;
	SequencerPatternType pattern;
	uint8_t lengthCounter;
	
	void (*next)(struct _tSequencer32 *self);
	void (*setPattern)(struct _tSequencer32 *self, SequencerPatternType type);
	
	int (*getNumNotes)(struct _tSequencer32 *self);
	
	uint16_t (*get)(struct _tSequencer32 *self, uint8_t step, StepParameterType param);
	uint16_t (*set)(struct _tSequencer32 *self, uint8_t step, StepParameterType param, uint16_t value);

	
} tSequencer32;

int tSequencer32Init(tSequencer32 *sequencer);


#endif /* SEQUENCER_H_ */