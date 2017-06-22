/*
 * IncFile1.h
 *
 * Created: 6/21/2017 4:47:16 PM
 *  Author: Jeff Snyder
 */ 


#ifndef TUNING_H_
#define TUNING_H_

#include <asf.h>

extern uint8_t tuning;

extern uint64_t scaledoctaveDACvalue;
extern signed char transpose;

const uint32_t tunings[99][129];	

extern uint16_t tuningDACTable[128];


void loadTuning(void);
void computeTuningDACTable(void);
unsigned short calculateDACvalue(uint8_t noteVal);


#endif /* TUNING_H_ */