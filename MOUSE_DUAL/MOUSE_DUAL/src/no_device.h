/*
 * IncFile1.h
 *
 * Created: 7/1/2017 10:46:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef NO_DEVICE_H_
#define NO_DEVICE_H_

#include "utilities.h"

#define EIGHT_BIT_DIV (RAND_MAX / 256)
#define TWELVE_BIT_DIV (RAND_MAX / 4096)
#define SIXTEEN_BIT_DIV (RAND_MAX / 65535)
#define ELEVEN_BIT_MAX (65535/2)
#define NUM_ELEMENTS_PER_PATTERN 17



typedef enum NoDeviceReadType
{
	RandomVoltage = 0,
	GatePulse,
	TriggerPulse,
	GateToggle
}	NoDeviceReadType;

typedef struct _tNoDevicePattern
{
	//save with preset
	uint16_t patterns[12][NUM_ELEMENTS_PER_PATTERN]; //12 outputs, each with a "number of beats active" value and then indexes for 32 total possible beats 
	NoDeviceReadType readType;
	uint8_t patternLength;
	BOOL allTheSameLength;
	//
	
	uint8_t trigCount[12];
	uint8_t patternCounter[12];
	uint8_t outputState[12];

} tNoDevicePattern;



void no_device_gate_in(void);

void noDeviceRandomVoltagePattern(BOOL sameLength);
void noDeviceGateOutPattern(BOOL sameLength);
void noDeviceGateTogglePattern(BOOL sameLength);
void noDeviceTrigOutPattern(BOOL sameLength);

void noDeviceCreateNewRandomPatterns(void);
void noDeviceSlightlyAlterRandomPatterns(void);

void triggerOnFirstDACOutput(void);

void tNoDevice_encode(tNoDevicePattern* const patterns, uint8_t* buffer);
void tNoDevice_decode(tNoDevicePattern* const patterns, uint8_t* buffer);



#endif /*  NO_DEVICE_H_ */