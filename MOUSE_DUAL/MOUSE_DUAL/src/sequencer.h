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

uint8_t SH_pattern_col_down[MAX_STEPS];
uint8_t SH_pattern_col_up[MAX_STEPS];
uint8_t SH_pattern_row_reverse[MAX_STEPS];
uint8_t SH_pattern_diag[MAX_STEPS];
uint8_t SH_pattern_diag_reverse[MAX_STEPS];

uint8_t patterns[5][MAX_STEPS];


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
	uint16_t fine;   
	uint16_t octave;
	uint16_t kbdhex;
	
	uint16_t pglide;
	uint16_t cvglide;
	
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
} tSequencer;

int Sequencer_init(tSequencer *seq, uint8_t maxLength);
void Sequencer_setPattern(tSequencer *seq, SequencerPatternType pat);
uint16_t Sequencer_set(tSequencer *seq, uint8_t step, StepParameterType paramType, uint16_t value);
uint16_t Sequencer_get(tSequencer *seq, uint8_t step, StepParameterType paramType);
int Sequencer_getHexFromStep(tSequencer *seq, uint8_t in);
uint8_t	Sequencer_getStepFromHex(tSequencer *seq, uint8_t in);
void Sequencer_next(tSequencer *seq);
int Sequencer_getNumNotes(tSequencer *seq);
int Sequencer_setMaxLength(tSequencer *seq, uint8_t maxLength);
void Sequencer_setOctave(tSequencer *seq, int8_t octave);
int Sequencer_getOctave(tSequencer *seq, int8_t octave);
void Sequencer_downOctave(tSequencer *seq);
void Sequencer_upOctave(tSequencer *seq);
int Sequencer_toggleStep(tSequencer *seq, uint8_t step);
int Sequencer_stepAdd(tSequencer *seq, uint8_t step);
int Sequencer_stepRemove(tSequencer *seq, uint8_t step);
void Sequencer_clearSteps(tSequencer *seq);

#endif /* SEQUENCER_H_ */