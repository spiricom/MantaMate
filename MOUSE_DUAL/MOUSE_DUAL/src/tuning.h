/*
 * IncFile1.h
 *
 * Created: 6/21/2017 4:47:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef TUNING_H_
#define TUNING_H_

#include <asf.h>

extern uint8_t globalTuning;
extern uint8_t tuningToUse;

extern uint8_t mantaUITunings[31];
extern uint8_t currentMantaUITuning;

extern uint64_t scaledoctaveDACvalue;

const uint32_t factoryTunings[100][50];	

extern uint32_t externalTuning[129];
extern uint32_t localTuningTemp[129];
extern uint16_t tuning8BitBuffer[768];

extern uint16_t tuningDACTable[128];

void loadTuning(uint8_t);
void computeTuningDACTable(TuningLoadLocation local_or_external);
unsigned short calculateDACvalue(uint8_t noteVal, TuningLoadLocation local_or_external);


#endif /* TUNING_H_ */