/*
 * notestack.c
 *
 * Created: 11/7/2016 9:08:50 AM
 *  Author: Jeff Snyder
 */ 

#include "notestack.h"

void tNoteStackAdd(tNoteStack *ns, uint8_t noteVal)
{
	uint8_t j;

	//first move notes that are already in the stack one position to the right
	for (j = ns->num; j > 0; j--)
	{
		ns->notestack[j] = ns->notestack[(j - 1)];
	}

	//then, insert the new note into the front of the stack
	ns->notestack[0] = noteVal;

	ns->num++;
}

// Remove noteVal.
int tNoteStackRemove(tNoteStack *ns, uint8_t noteVal)
{
	uint8_t j,k;
	int foundOne = 0;
	
	//it's a note-off, remove it from the stack
	//go through the notes that are currently held down to find the one that released
	for (j = 0; j < ns->num; j++)
	{
		//if it's the note that just got released
		if (ns->notestack[j] == noteVal)
		{
			for (k = 0; k < (ns->num - j); k++)
			{
				if ((k+j) >= (ns->size - 1))
				{
					ns->notestack[k + j] = -1;
				}
				else
				{
					ns->notestack[k + j] = ns->notestack[k + j + 1];
					if ((k+j) == (ns->num - 1))
					{
						ns->notestack[k + j + 1] = -1;
					}
				}
				
			}
			// in case it got put on the stack multiple times
			j--;
			ns->num--;
			foundOne = 1;
		}
	}
	return foundOne;
}

// Doesn't change size of data types 
void tNoteStackSetMaxSize(tNoteStack *ns, uint8_t size)
{
	if (size <= 0)
		ns->size = 1;
	else if (size <= 32)
		ns->size = size;
	else
		ns->size = 32;
		
	for (int i = size; i < 32; i++)
	{
		if (ns->notestack != -1)
		{
			ns->notestack[i] = -1;
			ns->num -= 1;
		}
	}
	
	if (ns->pos >= size)
	{
		ns->pos = 0;
	}
}

int tNoteStackToggle(tNoteStack *ns, uint8_t noteVal)
{
	uint8_t j,k;
	uint8_t foundOne = 0;
	//it's already in the stack, remove it from the stack
	// look through the stack
	
	foundOne = tNoteStackRemove(ns,noteVal);

	if (!foundOne)
	{
		tNoteStackAdd(ns,noteVal);
	}

	return !foundOne;
}

int tNoteStackClear(tNoteStack *ns)
{
	for (int i = 0; i < 32; i++)
	{
		ns->notestack[i] = -1;
	}
	ns->pos = 0;
	ns->num = 0;
}

// Next item in order of addition to stack. Return 0-31 if there is a next item to move to. Returns -1 otherwise.
int tNoteStackNext(tNoteStack *ns)
{
	int step = 0;
	if (ns->num != 0) // if there is at least one note in the stack
	{
		if (ns->pos > 0) // if you're not at the most recent note (first one), then go backward in the array (moving from earliest to latest)
		{
			ns->pos--;
		}
		else
		{
			ns->pos = (ns->num - 1); // if you are the most recent note, go back to the earliest note in the array
		}
		
		step = ns->notestack[ns->pos];
		return step;
	}
	else
	{
		return -1;
	}
}



int tNoteStackInit(tNoteStack *ns, uint8_t size)
{
	ns->num = 0;
	ns->pos = 0;
	ns->add = &tNoteStackAdd;
	ns->size = size;
	ns->remove = &tNoteStackRemove;
	ns->toggle = &tNoteStackToggle;
	ns->next = &tNoteStackNext;
	ns->setMaxSize = &tNoteStackSetMaxSize;
	return 0;
}
