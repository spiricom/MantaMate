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
	
	int octave;
	SequencerPatternType pattern;
	int maxLength;
	GlobalOptionType pitchOrTrigger;
	
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


#define MAX_VOICES 4

enum maps_t
{
	NO_MAP,
	WICKI_HAYDEN,
	HARMONIC,
	PIANO
};

unsigned long twelvetet[12];
unsigned long overtonejust[12];
unsigned long kora1[12];
unsigned long meantone[12];
unsigned long werckmeister1[12];
unsigned long werckmeister3[12];

unsigned long numTunings; // we need to think about how to structure this more flexibly. Should maybe be a Tunings struct that includes structs that define the tunings, and then we won't have to manually edit this. Also important for users being able to upload tunings via computer.

signed int whmap[48] ;

signed int harmonicmap[48];

signed int pianomap[48];

typedef struct _tKeyboard
{
	uint8_t numVoices;
	uint8_t numPlaying;
	
	tNoteStack notes;
	
	int polyVoiceBusy[MAX_VOICES];
	int polyVoiceNote[MAX_VOICES];
	
	int changevoice[MAX_VOICES];
	
	int currentNote;
	
	BOOL noteOn;
	BOOL noteOff;
	
	enum maps_t whichmap;
	
} tKeyboard;

void tKeyboard_init(tKeyboard* const k, int numVoices);
void tKeyboard_noteOn(tKeyboard* const k, int noteVal, uint8_t vel);
void tKeyboard_noteOff(tKeyboard* const k, uint8_t noteVal);


#endif /* SEQUENCER_H_ */