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
	
	for (int i = 0; i < 48; i++) direct->map[i] = -1;
	
	for (int i = 0; i < numOuts; i++)
	{
		direct->map[i] = i;
		direct->outs[i].hex = i;

		if (i < 3) tDirect_setOutputType(direct, i, DirectGate);
		else
		{
				tDirect_setOutputType(direct, i, DirectTrigger);
		}

		direct->outs[i].trigCount = 0;
	}
}

void tDirect_assignHexToOutput(tDirect* const direct, int hex, int output)
{
	// Deactivate last hex mapped to output.
	int lastHex = direct->outs[output].hex;
	direct->map[lastHex] = -1;
	
	// Assign hex to output.
	direct->outs[output].hex = hex;
	
	// Activate new hex assigned to output.
	direct->map[hex] = output;
}


DirectType tDirect_getOutputTypeForHex(tDirect* const direct, int hex)
{
	int output =  direct->map[hex];
	
	if (output < 0)		return DirectTypeNil;
	else				return direct->outs[output].type;

}

int tDirect_getOutputForHex(tDirect* const direct, int hex)
{
	return direct->map[hex];
}

void tDirect_setOutputType(tDirect* const direct, int output, DirectType type)
{
	direct->outs[output].type = type;
	direct->outs[output].color =	(type == DirectCV) ? Amber :
									(type == DirectGate) ? Red :
									(type == DirectTrigger) ? BothOn :
									Off;
									
	if (type == DirectTypeNil) 
	{
		direct->numActive--;
		direct->map[direct->outs[output].hex] = -1;
	}
	else if (type == DirectCV)
	{
		tIRampSetTime(&out[currentInstrument][output], 10);
	}
	else 
	{
		tIRampSetTime(&out[currentInstrument][output], 0);
	}
									
}

void tDirect_encode(tDirect* const direct, uint8_t* buffer)
{
	for (int i = 0; i < 12; i++)
	{
		buffer[i*2] = direct->outs[i].hex;
		buffer[(i*2) + 1] = direct->outs[i].type;
	}
}

// 24 bytes
void tDirect_decode(tDirect* const direct, uint8_t* buffer)
{
	DirectType type = DirectTypeNil;
	for (int i = 0; i < 12; i++)
	{
		direct->outs[i].hex = buffer[i*2];	
		type = buffer[(i*2) + 1];
		direct->outs[i].type = type;	
		direct->outs[i].color =	(type == DirectCV) ? Amber :
								(type == DirectGate) ? Red :
								(type == DirectTrigger) ? BothOn :
								Off;
	}

}