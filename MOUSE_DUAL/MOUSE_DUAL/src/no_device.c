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
		noDevicePatterns.readType = RandomVoltage;
		//all outputs are sample-and-hold random values except the first which is just a trigger on every clock
		triggerOnFirstDACOutput();

		for (int i = 1; i < 16; i++)
		{
			myRandom = (rand() / SIXTEEN_BIT_DIV);
			sendDataToOutput(i, globalCVGlide, myRandom);
		}
	}
	
	else if (preset_num == 1)
	{
		noDevicePatterns.readType = GatePulse;	
		//stream of random gates
		triggerOnFirstDACOutput();
		
		for (int i = 1; i < 16; i++)
		{
			myRandom = ((rand() / SIXTEEN_BIT_DIV) & 1);
			sendDataToOutput(i, 0, myRandom * 65535);
		}
	}
	
	else if (preset_num == 2)
	{
		noDevicePatterns.readType = TriggerPulse;	
		//stream of random triggers
		triggerOnFirstDACOutput();
		for (int i = 1; i < 16; i++)
		{
			myRandom = ((rand() / SIXTEEN_BIT_DIV) & 1);
			sendDataToOutput(i, 0, myRandom * 65535);
			noDevicePatterns.trigCount[i] = TRIGGER_TIMING;
		}
	}
	

	// Dividers ---  CONSECUTIVE INTEGERS
	
	//RANDOM VOLTAGE STYLE
	else if (preset_num == 3)
	{
		noDevicePatterns.readType = RandomVoltage;			
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			{
				sendDataToOutput(i, globalCVGlide, (rand()/SIXTEEN_BIT_DIV));
			}
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
	}
	
	
	//GATE STYLE
	else if (preset_num == 4)
	{
		noDevicePatterns.readType = GatePulse;	
		
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			{
				noDevicePatterns.outputState[i] = 1;
			}
			else
			{
				noDevicePatterns.outputState[i] = 0;
			}
			sendDataToOutput(i, 0, noDevicePatterns.outputState[i]*65535);
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
	}
	
	//TOGGLE STYLE
	else if (preset_num == 5)
	{
		noDevicePatterns.readType = GateToggle;	
		
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			{
				noDevicePatterns.outputState[i] = !noDevicePatterns.outputState[i]; //flip it!
			}
			sendDataToOutput(i, 0, noDevicePatterns.outputState[i]*65535);
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
	}
	
	//TRIGGER STYLE
	else if (preset_num == 6)
	{
		noDevicePatterns.readType = TriggerPulse;	
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (i+1)) == 0)
			sendDataToOutput(i, 0, 65535);
			noDevicePatterns.trigCount[i] = TRIGGER_TIMING;
		}
		if (dividerCount >= 12)
		{
			dividerCount = 0;
		}
	}
	
	// Dividers --- POWERS OF TWO
	
	// RANDOM VOLTAGE STYLE
	else if (preset_num == 7)
	{
		noDevicePatterns.readType = RandomVoltage;	
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				sendDataToOutput(i, globalCVGlide, (rand()/SIXTEEN_BIT_DIV));
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
	}
	
	//GATE STYLE
	else if (preset_num == 8)
	{
		noDevicePatterns.readType = GatePulse;	
		
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				sendDataToOutput(i, 0, 65535);
			}
			else
			{
				sendDataToOutput(i, 0, 0);
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
	}
	
	// TOGGLE STYLE
	else if (preset_num == 9)
	{
		noDevicePatterns.readType = GateToggle;	
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				noDevicePatterns.outputState[i] = !noDevicePatterns.outputState[i]; 
				sendDataToOutput(i, 0, noDevicePatterns.outputState[i] * 65535);
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
	}
	
	// TRIGGER STYLE
	else if (preset_num == 10)
	{
		noDevicePatterns.readType = TriggerPulse;	
		triggerOnFirstDACOutput();
		dividerCount++;
		for (int i = 1; i < 12; i++)
		{
			if ((dividerCount % (1<<i)) == 0)
			{
				sendDataToOutput(i, 0, 65535);
				noDevicePatterns.trigCount[i] = TRIGGER_TIMING;
			}
		}
		if (dividerCount >= 2048)
		{
			dividerCount = 0;
		}
	}
	
	else if (preset_num == 11)
	{
		noDeviceRandomVoltagePattern(TRUE);
	}
	else if (preset_num == 12)
	{
		noDeviceGateOutPattern(TRUE);
	}
	else if (preset_num == 13)
	{
		noDeviceGateTogglePattern(TRUE);
	}
	else if (preset_num == 14)
	{
		noDeviceTrigOutPattern(TRUE);
	}
	else if (preset_num == 15)
	{
		noDeviceRandomVoltagePattern(FALSE);
	}
	else if (preset_num == 16)
	{
		noDeviceGateOutPattern(FALSE);
	}
	else if (preset_num == 17)
	{
		noDeviceGateTogglePattern(FALSE);
	}
	else if (preset_num == 18)
	{
		noDeviceTrigOutPattern(FALSE);
	}
	
	else
	{
		if (noDevicePatterns.readType == RandomVoltage)
		{
			noDeviceRandomVoltagePattern(noDevicePatterns.allTheSameLength);
		}
		else if (noDevicePatterns.readType == GatePulse)
		{
			noDeviceGateOutPattern(noDevicePatterns.allTheSameLength);
		}
		else if (noDevicePatterns.readType == GateToggle)
		{
			noDeviceGateTogglePattern(noDevicePatterns.allTheSameLength);
		}
		else if (noDevicePatterns.readType == TriggerPulse)
		{
			noDeviceTrigOutPattern(noDevicePatterns.allTheSameLength);
		}
	}
	//first free preset for storage is preset 19
}


void noDeviceRandomVoltagePattern(BOOL sameLength)
{
	noDevicePatterns.readType = RandomVoltage;	
	noDevicePatterns.allTheSameLength = sameLength;
	tIRampSetDest(&out[0][0],65535);
	noDevicePatterns.trigCount[0] = TRIGGER_TIMING;
	for (int i = 1; i < 12; i++)
	{
		noDevicePatterns.patternCounter[i]++;
		if (sameLength == FALSE)
		{
			if ((noDevicePatterns.patternCounter[i] > noDevicePatterns.patterns[i][0]) || (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength))
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		else
		{
			if (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength)
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		sendDataToOutput(i, globalCVGlide, noDevicePatterns.patterns[i][noDevicePatterns.patternCounter[i] + 1]);
	}
}

void noDeviceGateOutPattern(BOOL sameLength)
{
	noDevicePatterns.readType = GatePulse;	
	noDevicePatterns.allTheSameLength = sameLength;
	tIRampSetDest(&out[0][0],65535);
	noDevicePatterns.trigCount[0] = TRIGGER_TIMING;
	for (int i = 1; i < 12; i++)
	{
		noDevicePatterns.patternCounter[i]++;
		if (sameLength == FALSE)
		{
			if ((noDevicePatterns.patternCounter[i] > noDevicePatterns.patterns[i][0]) || (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength))
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		else
		{
			if (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength)
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		sendDataToOutput(i, 0, (noDevicePatterns.patterns[i][noDevicePatterns.patternCounter[i] + 1] > ELEVEN_BIT_MAX)*65535);
	}
}

void noDeviceGateTogglePattern(BOOL sameLength)
{
	noDevicePatterns.readType = GateToggle;	
	noDevicePatterns.allTheSameLength = sameLength;
	tIRampSetDest(&out[0][0],65535);
	noDevicePatterns.trigCount[0] = TRIGGER_TIMING;
	for (int i = 1; i < 12; i++)
	{
		noDevicePatterns.patternCounter[i]++;
		if (sameLength == FALSE)
		{
			if ((noDevicePatterns.patternCounter[i] > noDevicePatterns.patterns[i][0]) || (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength))
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		else
		{
			if (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength)
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		if ((noDevicePatterns.patterns[i][noDevicePatterns.patternCounter[i] + 1] > ELEVEN_BIT_MAX)) //flip the output gate if a random pulse is in the array
		{
			noDevicePatterns.outputState[i] = !noDevicePatterns.outputState[i];
		}
		sendDataToOutput(i, 0, noDevicePatterns.outputState[i]*65535);
	}
}

void noDeviceTrigOutPattern(BOOL sameLength)
{
	noDevicePatterns.readType = TriggerPulse;	
	noDevicePatterns.allTheSameLength = sameLength;
	tIRampSetDest(&out[0][0],65535);
	noDevicePatterns.trigCount[0] = TRIGGER_TIMING;
	for (int i = 1; i < 12; i++)
	{
		noDevicePatterns.patternCounter[i]++;
		if (sameLength == FALSE)
		{
			if ((noDevicePatterns.patternCounter[i] > noDevicePatterns.patterns[i][0]) || (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength))
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		else
		{
			if (noDevicePatterns.patternCounter[i] >= noDevicePatterns.patternLength)
			{
				noDevicePatterns.patternCounter[i] = 0;
			}
		}
		
		int tempNewValue = (noDevicePatterns.patterns[i][noDevicePatterns.patternCounter[i] + 1] > ELEVEN_BIT_MAX);
		
		if ((tempNewValue > 0) && (noDevicePatterns.outputState[i] == 0))//if the value is TRUE and is was previously FALSE, send a trigger
		{
			
			sendDataToOutput(i, 0, (noDevicePatterns.patterns[i][noDevicePatterns.patternCounter[i] + 1] > ELEVEN_BIT_MAX)*65535);
			noDevicePatterns.trigCount[i] = TRIGGER_TIMING;
		}
		noDevicePatterns.outputState[i] = tempNewValue;

	}
}


void noDeviceCreateNewRandomPatterns(void)
{
	for (int i = 0; i < 12; i++)
	{
		noDevicePatterns.patterns[i][0] = (((rand() >> 15) % 31) + 2); //random number between 2 and 32
		for (int j = 1; j < NUM_ELEMENTS_PER_PATTERN; j++)
		{
			noDevicePatterns.patterns[i][j] = (uint16_t)(rand() / SIXTEEN_BIT_DIV); // a random 16-bit integer
		}
	}
}

void noDeviceSlightlyAlterRandomPatterns(void)
{
	for (int i = 0; i < 12; i++)
	{
		if (((rand()>>15) & 1) == 1) //roll one die to see if we should change this output's pattern at all (50% chance)
		{
			
			if (((rand()>>15) & 1) == 1) //roll one die to see if you should increment the pattern length at all
			{
				if (((rand()>>15) & 1) == 1) //roll a second die to see which direction   1 == go up
				{
					if (noDevicePatterns.patterns[i][0] < 32)
					{
						noDevicePatterns.patterns[i][0]++;
					}
				}
				else // zero == go down
				{
					if (noDevicePatterns.patterns[i][0] > 2)
					{
						noDevicePatterns.patterns[i][0]--;
					}
				}
			}
		
			for (int j = 1; j < NUM_ELEMENTS_PER_PATTERN; j++)
			{
				int myIncrement = 0;
				if (((rand()>>15) % 2) == 0) //roll die to see if it changes
				{
					if (((rand()>>15) & 1) == 1) //roll die to see if it goes up or down. 1 == up
					{
						myIncrement = ((rand() >> 15) % 16383);
					}
					else
					{
						myIncrement = -((rand() >> 15) % 16383);
					}
				}
			
				noDevicePatterns.patterns[i][j] += myIncrement; // it will overflow but that's OK
			}
			
		}
	}
}

void triggerOnFirstDACOutput(void)
{
		tIRampSetDest(&out[0][0], 65535);
		noDevicePatterns.trigCount[0] = TRIGGER_TIMING;
}

void tNoDevice_encode(tNoDevicePattern* nodevice, uint8_t* buffer)
{
	buffer[0] =  nodevice->readType;
	buffer[1] =  nodevice->patternLength;
	buffer[2] =  nodevice->allTheSameLength;
	int index_count = 3;
	
	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < NUM_ELEMENTS_PER_PATTERN; j++)
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
	nodevice->readType = buffer[0];
	nodevice->patternLength = buffer[1];
	nodevice->allTheSameLength = buffer[2];
	int index_count = 3;
	
	for (int i = 0; i < 12; i++)
	{
		for (int j = 0; j < NUM_ELEMENTS_PER_PATTERN; j++)
		{
			nodevice->patterns[i][j] = (buffer[index_count] << 8) +  buffer[index_count+1];
			index_count = (index_count + 2);
		}
		nodevice->patternCounter[i] = 0;
		nodevice->outputState[i] = 0;
		nodevice->trigCount[i] = 0;
	}
}
