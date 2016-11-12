/*
 * notestack.h
 *
 * Created: 11/7/2016 9:09:14 AM
 *  Author: Jeff Snyder
 */ 
#include "stdint.h"

#ifndef NOTESTACK_H_
#define NOTESTACK_H_

typedef struct _tNoteStack
{
	// max size 32
	
	// -1 in unsigned notestack is actually going to represented as 255. may cause issues if we aren't careful. should change.
	uint8_t notestack[32];
	uint8_t pos;
	uint8_t num;
	uint8_t size;
	
	void (*setMaxSize)(struct _tNoteStack *self, uint8_t size);
	
	void (*add)(struct _tNoteStack *self, uint8_t note);
	void (*remove)(struct _tNoteStack *self, uint8_t note);
	void (*clear)(struct _tNoteStack *self);
	
	int (*toggle)(struct _tNoteStack *self, uint8_t note);
	int (*next)(struct _tNoteStack *self); 
	
} tNoteStack;

int tNoteStackInit(tNoteStack *notestack, uint8_t size);



#endif /* NOTESTACK_H_ */