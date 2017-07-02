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
	uint32_t myRandom;
	if (preset_num == 0)
	{
		//all outputs are sample-and-hold random values
		myRandom = (rand() / SIXTEEN_BIT_DIV);
		tIRampSetDest(&out[0][0], myRandom);
		myRandom = (rand() / SIXTEEN_BIT_DIV);
		tIRampSetDest(&out[0][3], myRandom);
		myRandom = (rand() / SIXTEEN_BIT_DIV);
		tIRampSetDest(&out[1][0], myRandom);
		myRandom = (rand() / SIXTEEN_BIT_DIV);
		tIRampSetDest(&out[1][3], myRandom);

		for (int i = 0; i < 2; i++)
		{
			myRandom = (rand() / TWELVE_BIT_DIV);
			tIRampSetDest(&out[i][1], myRandom);
			myRandom = (rand() / TWELVE_BIT_DIV);
			tIRampSetDest(&out[i][2], myRandom);
			myRandom = (rand() / TWELVE_BIT_DIV);
			tIRampSetDest(&out[i][4], myRandom);
			myRandom = (rand() / TWELVE_BIT_DIV);
			tIRampSetDest(&out[i][5], myRandom);
		}
	}
	
	else if (preset_num == 1)
	{

		tIRampSetDest(&out[0][0], coinToss() * 65535);
		tIRampSetDest(&out[0][3], coinToss() * 65535);
		tIRampSetDest(&out[1][0], coinToss() * 65535);
		tIRampSetDest(&out[1][3], coinToss() * 65535);
		for (int i = 0; i < 2; i++)
		{
			tIRampSetDest(&out[i][1],coinToss() * 4095);
			tIRampSetDest(&out[i][2],coinToss() * 4095);
			tIRampSetDest(&out[i][4],coinToss() * 4095);
			tIRampSetDest(&out[i][5],coinToss() * 4095);
		}
	}
	
}

uint8_t coinToss(void)
{
	int32_t tempRandom = rand();
	if (tempRandom > (RAND_MAX / 2))
	{
		return 1;
	}
	else
	{
		return 0;
	}
}