/*
 * IncFile1.h
 *
 * Created: 7/1/2017 10:46:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef NO_DEVICE_H_
#define NO_DEVICE_H_

#include "main.h"

#define EIGHT_BIT_DIV (RAND_MAX / 256)
#define TWELVE_BIT_DIV (RAND_MAX / 4096)
#define SIXTEEN_BIT_DIV (RAND_MAX / 65535)


uint8_t noDeviceTrigCount[12];

void no_device_gate_in(void);

void setRampsWithDividerValsPlusTrig(void);
void setRampsWithDividerVals(void);

#endif /*  NO_DEVICE_H_ */