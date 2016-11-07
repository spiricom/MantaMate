/*
 * notestack.c
 *
 * Created: 11/7/2016 9:08:50 AM
 *  Author: Jeff Snyder
 */ 

#include "notestack.h"

void tNoteStack32Add(tNoteStack32 *ns, uint8_t noteVal)
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

void tNoteStack32Remove(tNoteStack32 *ns, uint8_t noteVal)
{
	uint8_t j,k;
	
	//it's a note-off, remove it from the stack
	//go through the notes that are currently held down to find the one that released
	for (j = 0; j < ns->num; j++)
	{
		//if it's the note that just got released
		if (ns->notestack[j] == noteVal)
		{
			for (k = 0; k < (ns->num - j); k++)
			{
				ns->notestack[k + j] = ns->notestack[k + j + 1];
				//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
				if (k == ((ns->num - j) - 1))
				ns->notestack[k + j + 1] = -1;
			}
			// in case it got put on the stack multiple times
			j--;
			ns->num--;
		}
	}
}

int tNoteStack32Toggle(tNoteStack32 *ns, uint8_t noteVal)
{
	uint8_t j,k;
	uint8_t foundOne = 0;
	//it's already in the stack, remove it from the stack
	// look through the stack
	for (j = 0; j < ns->num; j++)
	{
		//if you found it
		if (ns->notestack[j] == noteVal)
		{
			for (k = 0; k < (ns->num - j); k++)
			{
				ns->notestack[k + j] = ns->notestack[k + j + 1];
				//if it's the last one, write negative 1 beyond it (it's already been copied to the position to the left of it)
				if (k == ((ns->num - j) - 1))
				ns->notestack[k + j + 1] = -1;
			}
			// in case it got put on the stack multiple times
			j--;
			ns->num--;
			foundOne = 1;
		}
	}
	if (!foundOne)
	{
		tNoteStack32Add(ns,noteVal);
	}

	return !foundOne;
}

// Return 0-31 if should move to next step. Returns -1 otherwise.
int tNoteStack32Next(tNoteStack32 *ns)
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



int tNoteStack32Init(tNoteStack32 *ns)
{
	ns->num = 0;
	ns->pos = 0;
	ns->add = &tNoteStack32Add;
	ns->remove = &tNoteStack32Remove;
	ns->toggle = &tNoteStack32Toggle;
	ns->next = &tNoteStack32Next;
	return 0;
}
