/*
 * hex.h
 *
 * Created: 6/26/2017 3:49:06 PM
 *  Author: Mike Mulshine
 */ 


#ifndef HEX_H_
#define HEX_H_

#include "utilities.h"

typedef struct _tHex
{
	int16_t pitch; // mapped to which pitchclass
	MantaLEDColor color; // hex color
	BOOL active; // meaning, is assigned to active voice
	
} tHex;

void tHex_init(tHex* const hex, int which);

void tHex_setMap(tHex* const hex, int map, MantaLEDColor color);



#endif /* HEX_H_ */