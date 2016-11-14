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

#define MAX_STEPS 32

uint8_t pattern_col_down[MAX_STEPS];
uint8_t pattern_col_up[MAX_STEPS];
uint8_t pattern_row_reverse[MAX_STEPS];

uint8_t pattern_diag[MAX_STEPS];
uint8_t pattern_diag_reverse[MAX_STEPS];

typedef enum SequencerPatternType {
	LeftRightRowDown,
	LeftRightRowUp,
	LeftRightDiagDown,
	LeftRightDiagUp,
	LeftRightColDown,
	RightLeftColUp,
	RandomWalk,
	OrderTouch,
	RecordTouch,
	
	//not using atm
	RightLeftRowDown,
	RightLeftRowUp,
	RightLeftDiagDown,
	RightLeftDiagUp,
	LeftRightColUp,
	RightLeftColDown,
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

typedef struct _tSequencer
{

	tStep step[MAX_STEPS];
	int maxLength;
	int phasor;
	int octave;
	int currentStep,prevStep;
	int stepGo;
	tNoteStack notestack;
	SequencerPatternType pattern;
	uint8_t lengthCounter;
	
	void (*next)(struct _tSequencer *self);
	int (*toggle)(struct _tSequencer *self, uint8_t step);
	void (*setPattern)(struct _tSequencer *self, SequencerPatternType type);
	int (*getNumNotes)(struct _tSequencer *self);
	int (*setMaxLength)(struct _tSequencer *self, uint8_t maxLength);
	void (*setOctave)(struct _tSequencer *self, int8_t octave);
	void (*downOctave)(struct _tSequencer *self);
	void (*upOctave)(struct _tSequencer *self);
	
	uint16_t (*get)(struct _tSequencer *self, uint8_t step, StepParameterType param);
	uint16_t (*set)(struct _tSequencer *self, uint8_t step, StepParameterType param, uint16_t value);

	
} tSequencer;

int tSequencerInit(tSequencer *sequencer, uint8_t maxLength);


#endif /* SEQUENCER_H_ */