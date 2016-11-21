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
#include "utilities.h"



uint8_t pattern_col_down[MAX_STEPS];
uint8_t pattern_col_up[MAX_STEPS];
uint8_t pattern_row_reverse[MAX_STEPS];

uint8_t pattern_diag[MAX_STEPS];
uint8_t pattern_diag_reverse[MAX_STEPS];


typedef struct _tStep
{
	    
	// Pitch and Trigger step parameters
	uint8_t toggled; 
	uint16_t length;  
	uint16_t cv1;       
	uint16_t cv2; 
	   
	// Pitch only
	uint8_t note;   
	uint16_t cv3;       
	uint16_t cv4;      
	uint16_t pitch;   
	uint16_t octave;
	uint16_t kbdhex; 
	
	// Trigger only
	uint8_t on[4];
	
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

int tSequencerInit(tSequencer *tseq, uint8_t maxLength);


#endif /* SEQUENCER_H_ */