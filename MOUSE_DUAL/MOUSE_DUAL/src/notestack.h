/*
 * notestack.h
 *
 * Created: 11/7/2016 9:09:14 AM
 *  Author: Jeff Snyder
 */ 
#include "stdint.h"

#ifndef NOTESTACK_H_
#define NOTESTACK_H_

#define MAX_NUM_NOTES 48
typedef struct _tNoteStack
{
	// max size 32
	
	// -1 in unsigned notestack is actually going to represented as 255. may cause issues if we aren't careful. should change.
	int notestack[MAX_NUM_NOTES];
	uint8_t pos;
	uint8_t size;
	uint8_t capacity;
	
} tNoteStack;

int		tNoteStack_init					(tNoteStack *notestack, uint8_t size);
void	tNoteStack_setCapacity			(tNoteStack* const, uint8_t cap);
int		tNoteStack_addIfNotAlreadyThere	(tNoteStack* const, uint8_t note);
void	tNoteStack_add					(tNoteStack* const, uint8_t note);
int		tNoteStack_remove				(tNoteStack* const, uint8_t note);
void	tNoteStack_clear				(tNoteStack* const);
uint8_t tNoteStack_first				(tNoteStack* const);
int		tNoteStack_getSize				(tNoteStack* const);
int		tNoteStack_contains				(tNoteStack* const, uint8_t note);
int		tNoteStack_next					(tNoteStack* const);
int		tNoteStack_get					(tNoteStack* const, int which);


#endif /* NOTESTACK_H_ */