/*
 * NoDevice.c
 *
 * Created: 7/1/2017 10:46:34 PM
 *  Author: Jeff Snyder
 */ 

//functions for when there is no device or host plugged into either USB port.

#include "no_device.h"


void no_device_gate_in(void)
{
	uint32_t myRandom = (rand() + rand());
	tIRampSetDest(&out[0][0], myRandom);
	myRandom = (rand() + rand());
	tIRampSetDest(&out[0][3], myRandom);
	myRandom = (rand() + rand());
	tIRampSetDest(&out[1][0], myRandom);
	myRandom = (rand() + rand());
	tIRampSetDest(&out[1][3], myRandom);
	
	for (int i = 0; i < 5; i++)
	{
		myRandom = (rand() >> 3);
		tIRampSetDest(&out[0][i+1], myRandom);
		myRandom = (rand() >> 3);
		tIRampSetDest(&out[1][i+1], myRandom);
	}
}