/*
 * NoDevice.c
 *
 * Created: 7/1/2017 10:46:34 PM
 *  Author: Jeff Snyder
 */ 

//functions for when there is no device or host plugged into either USB port.

#include "no_device.h"
#include "main.h"

uint16_t dividerCount = 0;
uint8_t dividerValues[12] = {0};
uint8_t	noDeviceTrigCount[12] ={0};

void no_device_gate_in(void)
{
	if (!no_device_mode_active)
	{
		return;
	}
	
	
	uint32_t myRandom;
	if (preset_num == 0)
	{
		//all outputs are sample-and-hold random values except the first which is just a trigger on every clock
		tIRampSetDest(&out[0][0], 65535);
		noDevicePatterns.trigCount[0] = 8;
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
		//first output is a clock trigger
		tIRampSetDest(&out[0][0], 65535);
		noDevicePatterns.trigCount[0] = 8;
		tIRampSetDest(&out[0][3], (rand() > (RAND_MAX / 2)) * 65535);
		tIRampSetDest(&out[1][0], (rand() > (RAND_MAX / 2)) * 65535);
		tIRampSetDest(&out[1][3], (rand() > (RAND_MAX / 2)) * 65535);
		for (int i = 0; i < 2; i++)
		{
			tIRampSetDest(&out[i][1],(rand() > (RAND_MAX / 2)) * 4095);
			tIRampSetDest(&out[i][2],(rand() > (RAND_MAX / 2)) * 4095);
			tIRampSetDest(&out[i][4],(rand() > (RAND_MAX / 2)) * 4095);
			tIRampSetDest(&out[i][5],(rand() > (RAND_MAX / 2)) * 4095);
		}
	}
	

	// divider triggers
	else if (preset_num == 2)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			{
				dividerValues[i] = 1;
				noDevicePatterns.trigCount[i] = 8;
			}
			else
			{
				dividerValues[i] = 0;
				noDevicePatterns.trigCount[i] = 0;
			}
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
		setRampsWithDividerVals();
	}
	
	else if (preset_num == 3)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			{
				dividerValues[i] = 1;
			}
			else
			{
				dividerValues[i] = 0;
			}
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
		setRampsWithDividerValsPlusTrig();
	}
	
	// divider gate/toggles
	else if (preset_num == 4)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			dividerValues[i] = !dividerValues[i];
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
		setRampsWithDividerVals();
	}
	
	// divider triggers
	else if (preset_num == 5)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				dividerValues[i] = 1;
				noDevicePatterns.trigCount[i] = 8;
			}
			else
			{
				dividerValues[i] = 0;
				noDevicePatterns.trigCount[i] = 0;
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
		setRampsWithDividerVals();
	}
	
	else if (preset_num == 6)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				dividerValues[i] = 1;
			}
			else
			{
				dividerValues[i] = 0;
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
		setRampsWithDividerValsPlusTrig();
	}
	
	// divider gate/toggles
	else if (preset_num == 7)
	{
		dividerCount++;
		for (int i = 0; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			dividerValues[i] = !dividerValues[i];
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
		setRampsWithDividerVals();
	}
}

void setRampsWithDividerValsPlusTrig(void)
{
	tIRampSetDest(&out[0][0], 65535);
	noDevicePatterns.trigCount[0] = 8;
	tIRampSetDest(&out[0][1],  dividerValues[1] * 4095);
	tIRampSetDest(&out[0][2],  dividerValues[2] * 4095);
	tIRampSetDest(&out[0][3],  dividerValues[3] * 65535);
	tIRampSetDest(&out[0][4], dividerValues[4] * 4095);
	tIRampSetDest(&out[0][5], dividerValues[5] * 4095);
	tIRampSetDest(&out[1][0], dividerValues[6] * 65535);
	tIRampSetDest(&out[1][1], dividerValues[7] * 4095);
	tIRampSetDest(&out[1][2], dividerValues[8] * 4095);
	tIRampSetDest(&out[1][3], dividerValues[9] * 65535);
	tIRampSetDest(&out[1][4], dividerValues[10] * 4095);
	tIRampSetDest(&out[1][5], dividerValues[11] * 4095);
}

void setRampsWithDividerVals(void)
{
	tIRampSetDest(&out[0][0],  dividerValues[0] * 65535);
	tIRampSetDest(&out[0][1],  dividerValues[1] * 4095);
	tIRampSetDest(&out[0][2],  dividerValues[2] * 4095);
	tIRampSetDest(&out[0][3],  dividerValues[3] * 65535);
	tIRampSetDest(&out[0][4], dividerValues[4] * 4095);
	tIRampSetDest(&out[0][5], dividerValues[5] * 4095);
	tIRampSetDest(&out[1][0], dividerValues[6] * 65535);
	tIRampSetDest(&out[1][1], dividerValues[7] * 4095);
	tIRampSetDest(&out[1][2], dividerValues[8] * 4095);
	tIRampSetDest(&out[1][3], dividerValues[9] * 65535);
	tIRampSetDest(&out[1][4], dividerValues[10] * 4095);
	tIRampSetDest(&out[1][5], dividerValues[11] * 4095);
}



void tNoDevice_encode(tNoDevicePattern* nodevice, uint8_t* buffer)
{
	buffer[0] =  nodevice->readType;

	int index_count = 1;
	
	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			buffer[index_count] =  nodevice->patterns[i][j] >> 8;
			index_count++;
			buffer[index_count] = nodevice->patterns[i][j] & 0xff;
			index_count++;
		}
	}
}

void tNoDevice_decode(tNoDevicePattern* nodevice, uint8_t* buffer)
{
	buffer[0] =  nodevice->readType;
	int index_count = 1;
	
	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < 17; j++)
		{
			nodevice->patterns[i][j] = (buffer[index_count] << 8) +  buffer[index_count+1];
			index_count++;
			index_count++;
		}
	}
}
