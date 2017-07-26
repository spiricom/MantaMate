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
	direct->numActive = 0;
	
	for (int i = 0; i < 48; i++)
	{
		direct->hexes[i].output = -1;
		
		direct->hexes[i].type = DirectTypeNil;
		
		direct->hexes[i].trigCount = 0;
	}
	
	for (int i = 0; i < 2; i++)
	{
		direct->sliders[i].output = -1;
		
		direct->sliders[i].type = DirectTypeNil;
		
		direct->sliders[i].value = 0;
	}
	
	tDirect_setConfiguration(direct, DirectMixedOne);
		
}

void tDirect_setOutput(tDirect* const direct, int hex, int output)
{
	// Assign hex to output.
	if (hex < 48)
	{
		if (output < 0) direct->hexes[hex].type = DirectTypeNil;
		direct->hexes[hex].output = output;
	}
	else if (hex < 50)
	{
		int whichSlider = (hex-48);
		if (output < 0) direct->sliders[whichSlider].type = DirectTypeNil;
		direct->sliders[whichSlider].output = output;
	}
}


int tDirect_getOutput(tDirect* const direct, int hex)
{
	return ((hex < 48) ? direct->hexes[hex].output : direct->sliders[(hex-48)].output);
}


DirectType tDirect_getType(tDirect* const direct, int hex)
{
	return ((hex < 48) ? direct->hexes[hex].type : direct->sliders[(hex-48)].type);
}

void tDirect_setConfiguration(tDirect* const direct, DirectConfiguration which)
{
	tDirect_blank(direct);
	
	direct->numActive = direct->numOuts;
	
	if (which == DirectMixedOne)
	{
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[34+i].type = DirectGate;
			direct->hexes[34+i].output = i;
		}
		
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[18+i].type = DirectCV;
			direct->hexes[18+i].output = i + ((direct->numOuts == 12) ? 6 : 3);
		}
		
		if (direct->numOuts == 12)
		{
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[26+i].type = DirectTrigger;
				direct->hexes[26+i].output = i+3;
			}
			
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[10+i].type = DirectCV;
				direct->hexes[10+i].output = i+9;
			}
		}
	}
	else if (which == DirectMixedTwo)
	{
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[34+i].type = DirectGate;
			direct->hexes[34+i].output = i;
		}
		
		if (direct->numOuts == 6)
		{
			direct->hexes[18].type = DirectCV;
			direct->hexes[18].output = 3;
			
			direct->sliders[SliderOne].type = DirectCV;
			direct->sliders[SliderOne].output = 4;
			
			direct->sliders[SliderTwo].type = DirectCV;
			direct->sliders[SliderTwo].output = 5;
		}
		else if (direct->numOuts == 12)
		{
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[18+i].type = DirectCV;
				direct->hexes[18+i].output = i + ((direct->numOuts == 12) ? 6 : 3);
			}
			
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[26+i].type = DirectTrigger;
				direct->hexes[26+i].output = i+3;
			}
			
			direct->hexes[10].type = DirectCV;
			direct->hexes[10].output = 9;
			
			direct->sliders[SliderOne].type = DirectCV;
			direct->sliders[SliderOne].output = 10;
			
			direct->sliders[SliderTwo].type = DirectCV;
			direct->sliders[SliderTwo].output = 11;
		}
		
	}
	else 
	{
		DirectType type =	(which == DirectAllCVs) ? DirectCV :
							(which == DirectAllGates) ? DirectGate :
							(which == DirectAllTriggers) ? DirectTrigger :
							DirectTypeNil;
							
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[34+i].type = type;
			direct->hexes[34+i].output = i;
		}
		
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[18+i].type = type;
			direct->hexes[18+i].output = i + ((direct->numOuts == 12) ? 6 : 3);
		}
		
		if (direct->numOuts == 12)
		{
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[26+i].type = type;
				direct->hexes[26+i].output = i+3;
			}
			
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[10+i].type = type;
				direct->hexes[10+i].output = i+9;
			}
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
	
	for (int i = 0; i < 2; i++)
	{
		direct->sliders[i].output = -1;
		direct->sliders[i].type = DirectTypeNil;
		direct->sliders[i].value = 0;
	}
	
	direct->numActive = 0;
}

void tDirect_setType(tDirect* const direct, int hex, DirectType newType)
{
	DirectType oldType = (hex<48) ? (direct->hexes[hex].type) : (direct->sliders[(hex-48)].type);
	
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
	
	if (hex < 48)
	{
		direct->hexes[hex].type = newType;
	}
	else if (hex < 50)
	{
		direct->sliders[(hex-48)].type = newType;
	}
										
}


void initDirectMixedOne(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectMixedOne);

}

void initDirectMixedTwo(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectMixedTwo);

}


void initDirectAllTriggers(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllTriggers);
	
}

void initDirectAllGates(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllGates);
	
}

void initDirectAllCVs(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllCVs);
	
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