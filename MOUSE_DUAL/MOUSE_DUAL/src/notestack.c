/*
 * notestack.c
 *
 * Created: 11/7/2016 9:08:50 AM
 *  Author: Jeff Snyder
 */ 

#include "notestack.h"


// If stack contains note, returns index. Else returns -1;
int tNoteStackContains(tNoteStack *ns, uint8_t noteVal)
{
	for (int i = 0; i < ns->size; i++)
	{
		if (ns->notestack[i] == noteVal)	return i;
	}
	return -1;
}

void tNoteStackAdd(tNoteStack *ns, uint8_t noteVal)
{
	//first move notes that are already in the stack one position to the right
	for (int i = ns->size; i > 0; i--)
	{
		ns->notestack[i] = ns->notestack[(i - 1)];
	}

	//then, insert the new note into the front of the stack
	ns->notestack[0] = noteVal;

	ns->size++;
}

int tNoteStackAddIfNotAlreadyThere(tNoteStack *ns, uint8_t noteVal)
{
	uint8_t j;
	
	int foundIndex = tNoteStackContains(ns, noteVal);
	int added = 0;

	if (foundIndex == -1)
	{
		//first move notes that are already in the stack one position to the right
		for (j = ns->size; j > 0; j--)
		{
			ns->notestack[j] = ns->notestack[(j - 1)];
		}

		//then, insert the new note into the front of the stack
		ns->notestack[0] = noteVal;

		ns->size++;
		
		added = 1;
	}
	
	return added;
}


// Remove noteVal. return 1 if removed, 0 if not
int tNoteStackRemove(tNoteStack *ns, uint8_t noteVal)
{
	uint8_t k;
	int foundIndex = tNoteStackContains(ns, noteVal);
	int removed = 0;
	
	if (foundIndex >= 0)
	{
		for (k = 0; k < (ns->size - foundIndex); k++)
		{
			if ((k+foundIndex) >= (ns->capacity - 1))
			{
				ns->notestack[k + foundIndex] = -1;
			}
			else
			{
				ns->notestack[k + foundIndex] = ns->notestack[k + foundIndex + 1];
				if ((k + foundIndex) == (ns->size - 1))
				{
					ns->notestack[k + foundIndex + 1] = -1;
				}
			}
			
		}
		// in case it got put on the stack multiple times
		foundIndex--;
		ns->size--;
		removed = 1;
	}

			

	return removed;
}

// Doesn't change size of data types 
void tNoteStackSetCapacity(tNoteStack *ns, uint8_t cap)
{
	if (cap <= 0)
		ns->capacity = 1;
	else if (cap <= 32)
		ns->capacity = cap;
	else
		ns->capacity = 32;
		
	for (int i = cap; i < 32; i++)
	{
		if (ns->notestack != -1)
		{
			ns->notestack[i] = -1;
			ns->size -= 1;
		}
	}
	
	if (ns->pos >= cap)
	{
		ns->pos = 0;
	}
}

int tNoteStackGetSize(tNoteStack *ns)
{
	return ns->size;
}

int tNoteStackClear(tNoteStack *ns)
{
	for (int i = 0; i < 32; i++)
	{
		ns->notestack[i] = -1;
	}
	ns->pos = 0;
	ns->size = 0;
}

// Next item in order of addition to stack. Return 0-31 if there is a next item to move to. Returns -1 otherwise.
int tNoteStackNext(tNoteStack *ns)
{
	int step = 0;
	if (ns->size != 0) // if there is at least one note in the stack
	{
		if (ns->pos > 0) // if you're not at the most recent note (first one), then go backward in the array (moving from earliest to latest)
		{
			ns->pos--;
		}
		else
		{
			ns->pos = (ns->size - 1); // if you are the most recent note, go back to the earliest note in the array
		}
		
		step = ns->notestack[ns->pos];
		return step;
	}
	else
	{
		return -1;
	}
}

uint8_t tNoteStackFirst(tNoteStack *ns)
{
	return ns->notestack[0];
}

int tNoteStackInit(tNoteStack *ns, uint8_t cap)
{
	ns->size = 0;
	ns->pos = 0;
	ns->capacity = cap;
	
	ns->next = &tNoteStackNext;
	ns->add = &tNoteStackAdd;
	ns->addIfNotAlreadyThere = &tNoteStackAddIfNotAlreadyThere;
	ns->remove = &tNoteStackRemove;
	ns->getSize = &tNoteStackGetSize;
	ns->contains = &tNoteStackContains;
	ns->clear = &tNoteStackClear;
	ns->first = &tNoteStackFirst;
	ns->setCapacity = &tNoteStackSetCapacity;
	return 0;
}
