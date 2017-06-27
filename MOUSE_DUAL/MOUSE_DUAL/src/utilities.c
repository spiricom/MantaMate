/*
 * utilities.c
 *
 * Created: 12/23/2016 4:18:34 PM
 *  Author: Mike Mulshine
 */ 

#include "utilities.h"

#define COMPUTE_INC() (r->time>0) ? (r->inc = ((r->dest-r->curr)/r->time * r->inv_sr_ms)*((float)r->samples_per_tick)) : (r->inc = (r->dest-r->curr))

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


#define ICOMPUTE_INC_LARGE() (r->time>0) ? (r->inc = (((distance_to_travel/r->time) / 1000) * r->inv_sr_us)) : (r->inc = distance_to_travel)
#define ICOMPUTE_INC_SMALL() (r->time>0) ? (r->inc = (((distance_to_travel/r->time) * r->inv_sr_us) / 1000)) : (r->inc = distance_to_travel)
#define ICOMPUTE_INC_VERY_SMALL() (r->time>0) ? (r->inc = ((((distance_to_travel * 1000) / r->time) * r->inv_sr_us) / 100000)) : (r->inc = distance_to_travel)

//I now assume that there are always 1 sample per tick
int tIRampInit(tIRamp *r, int32_t sr, int32_t time) {
	r->inv_sr_us = 1000000/sr; //should give how many microseconds per tick
	r->curr = 0;
	r->dest = 0;
	r->time = time;
	r->inc = 0;
	return 0;
}

uint16_t tIRampTick(tIRamp *r) {
	
	r->curr += r->inc;
	
	if (((r->curr >= r->dest) && (r->inc > 0)) || ((r->curr <= r->dest) && (r->inc < 0)))
	{
		r->inc = 0;
		r->curr = r->dest;
	}
	return (uint16_t) (r->curr >> 10); //get it back into the usual range from 0-65535
}

//ramp time can't be larger than 512
int tIRampSetTime(tIRamp *r, int32_t time) {
	r->time = time;
	int32_t distance_to_travel = r->dest-r->curr;
	if ((distance_to_travel > 4096000) || (distance_to_travel < -4096000))
	{
		ICOMPUTE_INC_LARGE();
	}
	else if  ((distance_to_travel < 4096) && (distance_to_travel > -4906))
	{
		ICOMPUTE_INC_VERY_SMALL();
	}
	else
	{
		ICOMPUTE_INC_SMALL();
	}
	return 0;
}

int tIRampSetDest(tIRamp *r, int32_t dest) {
	r->dest = dest << 10; //shift over 10 bits (equivalent to multiplying by 1024 to make it a larger number and give more precision - similar to multiplying by 1000 to move over the decimal place)
	int32_t distance_to_travel = r->dest-r->curr;
	if ((distance_to_travel > 4096000) || (distance_to_travel < -4096000))
	{
		ICOMPUTE_INC_LARGE();
	}
	else if  ((distance_to_travel < 4096) && (distance_to_travel > -4906))
	{
		ICOMPUTE_INC_VERY_SMALL();
	}
	else
	{
		ICOMPUTE_INC_SMALL();
	}
	return 0;
}