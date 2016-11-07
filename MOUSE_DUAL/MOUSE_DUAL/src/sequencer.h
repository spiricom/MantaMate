/*
 * sequencer.h
 *
 * Created: 11/7/2016 9:54:07 AM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_H_
#define SEQUENCER_H_

#include "stdint.h"

//#include "notestack.h"

	
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
	int currentStep;
	//tNoteStack32 notestack;
	
	
	void (*next)(struct _tNoteStack32 *self, uint8_t note);
	
} tSequencer32;

int tSequencer32Init(tSequencer32 *sequencer);


#endif /* SEQUENCER_H_ */