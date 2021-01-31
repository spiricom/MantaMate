/*
 * IncFile1.h
 *
 * Created: 6/21/2017 4:47:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef TUNING_H_
#define TUNING_H_

#include "utilities.h"

extern uint8_t globalTuning;
extern uint8_t tuningToUse;

extern uint8_t mantaUITunings[32];
extern uint8_t currentMantaUITuning;

extern uint64_t scaledoctaveDACvalue;

const uint32_t factoryTunings[100][50];	

extern uint32_t scaledSemitoneDACvalue;
extern uint32_t scaledOctaveDACvalue;
#define TUNING_8BIT_BUFFER_SIZE 520

extern uint32_t externalTuning[129];
extern uint8_t tuning8BitBuffer[TUNING_8BIT_BUFFER_SIZE];

typedef struct _tTuningTable
{
	uint16_t tuningDACTable[128];
	uint8_t cardinality;
}tTuningTable;


void loadTuning(uint8_t);
void computeTuningDACTable(tTuningTable* myTable, TuningLoadLocation local_or_external);
unsigned short calculateDACvalue(uint8_t noteVal, TuningLoadLocation local_or_external, uint16_t cardinality);


#endif /* TUNING_H_ */