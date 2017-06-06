/*
 * preset.h
 *
 * Created: 3/3/2017 11:10:20 AM
 *  Author: Mike Mulshine
 */ 


#ifndef PRESET_H_
#define PRESET_H_


#include "sequencer.h"

typedef struct _tMantaMatePreset
{
	// 90 pairs of sequencers in one of four states (PitchFull, PitchSplit, TriggerFull, TriggerSplit
	tSequencer sequencers[90][2]; 
	
} tMantaMatePreset;



#endif /* PRESET_H_ */