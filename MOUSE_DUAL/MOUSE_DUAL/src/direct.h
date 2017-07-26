/*
 * direct.h
 *
 * Created: 6/26/2017 3:46:24 PM
 *  Author: Mike Mulshine
 */ 


#ifndef DIRECT_H_
#define DIRECT_H_

#include "utilities.h"


#include "hex.h"

typedef enum DirectType
{
	DirectCV = 0,
	DirectTrigger,
	DirectGate,
	DirectTypeNil
	
} DirectType;


typedef struct _tDirectHex
{
	DirectType type;
	int output;
	
	int trigCount;
	
} tDirectHex;

typedef struct _tDirectSlider
{
	int output;
	DirectType type;
	uint16_t value;
	
} tDirectSlider;

typedef struct _tDirect
{
	// Encode this in preset
	tDirectHex hexes[48];
	tDirectSlider sliders[2];
	int numOuts;
	// ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~
	
	int numActive;
	
} tDirect;

void tDirect_init(tDirect* const direct, int numVoices);

void tDirect_setType(tDirect* const direct, int output, DirectType type);
DirectType tDirect_getType(tDirect* const direct, int hex);

void tDirect_setOutput(tDirect* const direct, int hex, int output);
int tDirect_getOutput(tDirect* const direct, int hex);

void tDirect_setConfiguration(tDirect* const direct, int which);

void tDirect_blank(tDirect* const direct);

void initMantaAllCV(void);

void initMantaAllGates(void);

void initMantaAllTriggers(void);

void initMantaCVAndGates(void);

void initMantaCVAndTriggers(void);


void tDirect_encode(tDirect* const direct, uint8_t* buffer);
void tDirect_decode(tDirect* const direct, uint8_t* buffer);

#endif /* DIRECT_H_ */