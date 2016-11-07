/*
 * notestack.h
 *
 * Created: 11/7/2016 9:09:14 AM
 *  Author: Jeff Snyder
 */ 
#include "stdint.h"

#ifndef NOTESTACK_H_
#define NOTESTACK_H_

typedef struct _tNoteStack32
{
	uint8_t notestack[32];
	uint8_t pos;
	uint8_t num;
	
	
	void (*add)(struct _tNoteStack32 *self, uint8_t note);
	void (*remove)(struct _tNoteStack32 *self, uint8_t note);
	
	int (*toggle)(struct _tNoteStack32 *self, uint8_t note);
	int (*next)(struct _tNoteStack32 *self); 
	
} tNoteStack32;

int tNoteStack32Init(tNoteStack32 *notestack);



#endif /* NOTESTACK_H_ */