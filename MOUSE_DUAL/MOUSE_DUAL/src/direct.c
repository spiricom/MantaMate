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
	else if (which == DirectMixedTwo)
	{
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[0+16*i].type = DirectGate;
			direct->hexes[0+16*i].output = i;
		}
		
		for (int i = 0; i < 3; i++)
		{
			direct->hexes[2+16*i].type = DirectCV;
			direct->hexes[2+16*i].output = 3+i;
		}
		
		if (direct->numOuts == 12)
		{
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[4+16*i].type = DirectTrigger;
				direct->hexes[4+16*i].output = i+3;
			}
			
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[6+16*i].type = DirectCV;
				direct->hexes[6+16*i].output = i+9;
			}
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
		
		if (direct->numOuts == 6)
		{
			direct->hexes[18].type = type;
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
				direct->hexes[18+i].type = type;
				direct->hexes[18+i].output = i + ((direct->numOuts == 12) ? 6 : 3);
			}
			
			for (int i = 0; i < 3; i++)
			{
				direct->hexes[26+i].type = type;
				direct->hexes[26+i].output = i+3;
			}
			
			direct->hexes[10].type = type;
			direct->hexes[10].output = 9;
			
			direct->sliders[SliderOne].type = DirectCV;
			direct->sliders[SliderOne].output = 10;
			
			direct->sliders[SliderTwo].type = DirectCV;
			direct->sliders[SliderTwo].output = 11;
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
	
	manta[InstrumentOne].type = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectMixedOne);
	
	for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
}

void initDirectMixedTwo(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	manta[InstrumentOne].type = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectMixedTwo);
	
	for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
}


void initDirectAllTriggers(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	manta[InstrumentOne].type = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllTriggers);
	
	for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
}

void initDirectAllGates(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	manta[InstrumentOne].type = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllGates);
	
	for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
}

void initDirectAllCVs(void)
{
	takeover = TRUE;
	takeoverType = DirectInstrument;
	
	manta[InstrumentOne].type = DirectInstrument;
	
	tDirect_init(&fullDirect, 12);
	
	tDirect_setConfiguration(&fullDirect, DirectAllCVs);
	
	for (int i = 0; i < 12; i++) sendDataToOutput(i, globalCVGlide, 0);
}

void tDirect_encode(tDirect* const direct, uint8_t* buffer)
{
	int idx = 0;
	for (int i = 0; i < 48; i++)
	{
		buffer[idx++] = direct->hexes[i].output;
		buffer[idx++] = direct->hexes[i].type;
	}
	
	for (int i = 0; i < 2; i++)
	{
		buffer[idx++] = direct->sliders[i].output;
		buffer[idx++] = direct->sliders[i].type;
		buffer[idx++] = direct->sliders[i].value >> 8;
		buffer[idx++] = direct->sliders[i].value & 0xff;
	}
	
	buffer[idx++] = direct->numActive;
	buffer[idx++] = direct->numOuts;
}

// 24 bytes
void tDirect_decode(tDirect* const direct, uint8_t* buffer)
{
	uint16_t highByte,lowByte;
	
	int idx = 0;
	for (int i = 0; i < 48; i++)
	{
		direct->hexes[i].output = buffer[idx++];
		direct->hexes[i].type = buffer[idx++];
	}
	
	for (int i = 0; i < 2; i++)
	{
		direct->sliders[i].output = buffer[idx++];
		direct->sliders[i].type = buffer[idx++];
		
		highByte = (buffer[idx++] << 8);
		lowByte = buffer[idx++];
		direct->sliders[i].value = highByte + lowByte;
	}
	
	direct->numActive = buffer[idx++];
	direct->numOuts = buffer[idx++];
}