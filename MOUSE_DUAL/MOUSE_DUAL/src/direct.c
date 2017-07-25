/*
 * direct.c
 *
 * Created: 6/26/2017 3:46:15 PM
 *  Author: Mike Mulshine
 */ 

#include "direct.h"
#include "main.h"

void tDirect_init(tDirect* const direct, int numOuts)
{
	direct->numOuts = numOuts;
	direct->numActive = numOuts;
	
	for (int i = 0; i < 48; i++)
	{
		direct->hexes[i].output = -1;
		
		direct->hexes[i].type = DirectTypeNil;
		
		direct->hexes[i].trigCount = 0;
	}
	
	tDirect_setConfiguration(direct, 0);
		
}

void tDirect_setOutput(tDirect* const direct, int hex, int output)
{
	// Assign hex to output.
	direct->hexes[hex].output = output;
}


int tDirect_getOutput(tDirect* const direct, int hex)
{
	return direct->hexes[hex].output;
}


DirectType tDirect_getType(tDirect* const direct, int hex)
{
	return direct->hexes[hex].type;
}

void tDirect_setConfiguration(tDirect* const direct, int which)
{
	tDirect_blank(direct);
	
	if (which == 0)
	{
		// DEFAULT DIRECT FOR NOW
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			if ((i%6) < 2)		direct->hexes[i].type = DirectTrigger;
			else if ((i%6) < 4) direct->hexes[i].type = DirectGate;
			else if ((i%6) < 6) direct->hexes[i].type = DirectCV;
		}
	}
	else if (which == 1)
	{
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			direct->hexes[i].type = DirectTrigger;
		}
	}
	else if (which == 2)
	{
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			direct->hexes[i].type = DirectGate;
		}
	}
	else if (which == 3)
	{
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			direct->hexes[i].type = DirectCV;
		}
	}
	else if (which == 4)
	{
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			if ((i%6) < 3)		direct->hexes[i].type = DirectTrigger;
			else if ((i%6) < 6) direct->hexes[i].type = DirectCV;
		}
	}
	else if (which == 5)
	{
		for (int i = 0; i < direct->numOuts; i++)
		{
			direct->hexes[i].output = i;
			
			if ((i%6) < 3)		direct->hexes[i].type = DirectGate;
			else if ((i%6) < 6) direct->hexes[i].type = DirectCV;
		}
	}
}

void tDirect_blank(tDirect* const direct)
{
	for (int i = 0; i < 48; i++)
	{
		direct->hexes[i].output = -1;
		direct->hexes[i].type = DirectTypeNil;
	}
	direct->numActive = 0;
}

void tDirect_setType(tDirect* const direct, int hex, DirectType newType)
{
	DirectType oldType = direct->hexes[hex].type;
	
	if (oldType == DirectTypeNil && newType != DirectTypeNil)		
	{
		if (++direct->numActive > direct->numOuts) 
		{
			direct->numActive = direct->numOuts;
			return;
		}
	}
	else if (oldType != DirectTypeNil && newType == DirectTypeNil)	
	{
		if (--direct->numActive < 0)
		{
			direct->numActive = 0;
		}
	}
	
	direct->hexes[hex].type = newType;
	
										
}


void initMantaAllCV(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	manta[InstrumentOne].type = DirectInstrument;
	fullDirect.numOuts = 12;
	for (int i = 0; i < 12; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectCV;
	}

}

void initMantaAllGates(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	manta[InstrumentOne].type = DirectInstrument;
	fullDirect.numOuts = 12;
	for (int i = 0; i < 12; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectGate;
	}

}

void initMantaAllTriggers(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	manta[InstrumentOne].type = DirectInstrument;
	fullDirect.numOuts = 12;
	for (int i = 0; i < 12; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectTrigger;
	}	
}

void initMantaCVAndGates(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	manta[InstrumentOne].type = DirectInstrument;
	fullDirect.numOuts = 12;
	for (int i = 0; i < 6; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectCV;
	}	
	for (int i = 0; i < 6; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectGate;
	}

}

void initMantaCVAndTriggers(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	manta[InstrumentOne].type = DirectInstrument;
	fullDirect.numOuts = 12;
	for (int i = 0; i < 6; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectCV;
	}	
	for (int i = 0; i < 6; i++)
	{
		fullDirect.hexes[i].output = i;
		fullDirect.hexes[i].type = DirectTrigger;
	}

}


void tDirect_encode(tDirect* const direct, uint8_t* buffer)
{

	for (int i = 0; i < 48; i++)
	{
		buffer[i*2] = direct->hexes[i].output;
		buffer[(i*2) + 1] = direct->hexes[i].type;
	}
	
	buffer[48] = direct->numOuts;
}

// 24 bytes
void tDirect_decode(tDirect* const direct, uint8_t* buffer)
{
	DirectType type = DirectTypeNil;
	for (int i = 0; i < 12; i++)
	{
		direct->hexes[i].output = buffer[i*2];	
		type = buffer[(i*2) + 1];
		direct->hexes[i].type = type;	
	}
	direct->numOuts = buffer[48];
}