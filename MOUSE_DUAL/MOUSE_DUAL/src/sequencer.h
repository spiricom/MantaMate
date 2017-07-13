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
	uint16_t cvglide;
	   
	// Pitch only
	uint8_t note;   
	uint16_t cv3;       
	uint16_t cv4;      
	uint16_t pitch;
	uint16_t fine;   
	uint16_t octave;
	uint16_t kbdhex;
	uint16_t pglide;
	;
	
	// Trigger only
	uint8_t on[4];
	
} tStep;


typedef enum SequencerDataType
{
	SeqOctave = 0, 
	SeqMaxLength,
	SeqPitchOrTrigger,
	SeqSteps,
	SequencerDataTypeNil
	
} SequencerDataType;

typedef struct _tSequencer
{
	
	tStep step[MAX_STEPS];
	int phasor;
	int currentStep,prevStep;
	int stepGo;
	tNoteStack notestack;
	uint8_t lengthCounter;
	
	uint8_t trigCount[5]; //doesn't need to be saved in a preset or composition, just for temporary handling of trigger off times
	
	int8_t octave;
	SequencerPatternType pattern;
	int maxLength;
	GlobalOptionType pitchOrTrigger;
	
	signed int transpose;
	
	MantaPlayMode playMode;
	
	int lastTouch;
	
} tSequencer;

int			tSequencer_init				(tSequencer* const, GlobalOptionType type, uint8_t maxLength);
void		tSequencer_next				(tSequencer* const);
int			tSequencer_toggleStep		(tSequencer* const, uint8_t step);
int			tSequencer_addStep			(tSequencer* const, uint8_t step);
int			tSequencer_removeStep		(tSequencer* const, uint8_t step);
void		tSequencer_setPattern		(tSequencer* const, SequencerPatternType type);
int			tSequencer_getNumNotes		(tSequencer* const);
void		tSequencer_setMaxLength		(tSequencer* const, uint8_t maxLength);
void		tSequencer_setOctave		(tSequencer* const, int8_t octave);
int			tSequencer_getOctave		(tSequencer* const);
void		tSequencer_downOctave		(tSequencer* const);
void		tSequencer_upOctave			(tSequencer* const);
void		tSequencer_clearSteps		(tSequencer* const);
int			tSequencer_getHexFromStep	(tSequencer* const, uint8_t step);
int			tSequencer_getStepFromHex	(tSequencer* const, uint8_t  hex);
uint16_t	tSequencer_getParameterValue(tSequencer* const, uint8_t step, StepParameterType param);
void		tSequencer_setParameterValue(tSequencer* const, uint8_t step, StepParameterType param, uint16_t value);

void		tSequencer_randomizePitch(tSequencer* const seq);
void		tSequencer_randomizeCV(tSequencer* const seq);
void		tSequencer_randomizeStepLength(tSequencer* const seq);
void		tSequencer_randomizeGlide(tSequencer* const seq);
void		tSequencer_randomizeToggled(tSequencer* const seq);
void		tSequencer_randomizeTrigger(tSequencer* const seq);
void		tSequencer_randomizeAll(tSequencer* const seq);

void        tSequencer_encode(tSequencer* const, uint16_t* sBuffer);
void        tSequencer_decode(tSequencer* const, uint16_t* sBuffer);





#endif /* SEQUENCER_H_ */