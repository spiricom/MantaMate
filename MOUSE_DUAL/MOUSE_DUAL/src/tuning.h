/*
 * IncFile1.h
 *
 * Created: 6/21/2017 4:47:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef TUNING_H_
#define TUNING_H_

#include "utilities.h"

extern uint8_t tuning;
extern uint8_t tuningToLoad;

extern uint64_t scaledoctaveDACvalue;

const uint32_t factoryTunings[100][50];	

extern uint32_t externalTuning[129];
extern uint32_t localTuningTemp[129];
extern uint16_t tuning8BitBuffer[768];

typedef struct _tTuningTable
{
	uint16_t tuningDACTable[128];
	int16_t cardinality;
}tTuningTable;


void loadTuning(void);
void computeTuningDACTable(tTuningTable* myTable, TuningLoadLocation local_or_external);
unsigned short calculateDACvalue(uint8_t noteVal, TuningLoadLocation local_or_external, uint16_t cardinality);


#endif /* TUNING_H_ */