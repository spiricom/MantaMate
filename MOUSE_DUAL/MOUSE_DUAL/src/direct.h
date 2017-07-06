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
	DirectCV,
	DirectTrigger,
	DirectGate,
	DirectTypeNil
	
} DirectType;


typedef struct _tDirectOutput
{
	int hex;
	
	DirectType type;
	
	MantaLEDColor color;
	
	int trigCount;
	
} tDirectOutput;

typedef struct _tDirect
{
	tDirectOutput outs[12];
	
	int numOuts;
	
	int numActive;
	
	int map[48];
	
} tDirect;

void tDirect_init(tDirect* const direct, int numVoices);

DirectType tDirect_getOutputTypeForHex(tDirect* const direct, int hex);

int tDirect_getOutputForHex(tDirect* const direct, int hex);

void tDirect_assignHexToOutput(tDirect* const direct, int hex, int output);

void tDirect_setOutputType(tDirect* const direct, int output, DirectType type);

#endif /* DIRECT_H_ */