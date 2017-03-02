/*
 * utilities.c
 *
 * Created: 12/23/2016 4:18:34 PM
 *  Author: Mike Mulshine
 */ 

#include "utilities.h"

#define COMPUTE_INC() r->inc = ((r->dest-r->curr)/r->time * r->inv_sr_ms)*((float)r->samples_per_tick)

int tRampSetTime(tRamp *r, float time) {
	r->time = time;
	COMPUTE_INC();
	return 0;
}

int tRampSetDest(tRamp *r, float dest) {
	r->dest = dest;
	COMPUTE_INC();
	return 0;
}

float tRampTick(tRamp *r) {
	
	r->curr += r->inc;
	
	if (((r->curr >= r->dest) && (r->inc > 0.0f)) || ((r->curr <= r->dest) && (r->inc < 0.0f))) 
	{
		r->inc = 0.0f;
		r->curr = r->dest;
	}	
		

	return r->curr;
}

int tRampInit(tRamp *r, float sr, uint16_t time, int samples_per_tick) {
	r->inv_sr_ms = 1.0f/(sr*0.001f);
	r->curr = 0.0f;
	r->dest = 0.0f;
	r->time = time;
	r->samples_per_tick = samples_per_tick;
	COMPUTE_INC();
	return 0;
}

