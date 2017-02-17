/*
 * notestack.h
 *
 * Created: 11/7/2016 9:09:14 AM
 *  Author: Jeff Snyder
 */ 
#include "stdint.h"

#ifndef NOTESTACK_H_
#define NOTESTACK_H_

typedef struct _tNoteStack {
		// max size 32
		
		// -1 in unsigned notestack is actually going to represented as 255. may cause issues if we aren't careful. should change.
		uint8_t notestack[32];
		uint8_t pos;
		uint8_t size;
		uint8_t capacity;
} tNoteStack;

int NoteStack_init(tNoteStack *ns, uint8_t cap);
int NoteStack_contains(tNoteStack *ns, uint8_t noteVal);
void NoteStack_add(tNoteStack *ns, uint8_t noteVal);
int NoteStack_remove(tNoteStack *ns, uint8_t noteVal);
void NoteStack_setCapacity(tNoteStack *ns, uint8_t cap);
int NoteStack_getSize(tNoteStack *ns);
int NoteStack_clear(tNoteStack *ns);
int NoteStack_next(tNoteStack *ns);
uint8_t NoteStack_first(tNoteStack *ns);



#endif /* NOTESTACK_H_ */