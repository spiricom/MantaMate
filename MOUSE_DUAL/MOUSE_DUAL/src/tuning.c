/*
 * CFile1.c
 *
 * Created: 6/21/2017 4:46:56 PM
 *  Author: Jeff Snyder
 */ 

#include "tuning.h"

uint8_t tuning = 0; //0-99

uint32_t scaledSemitoneDACvalue = 54613;
uint32_t scaledOctaveDACvalue = 655350;

signed char transpose = 0;
	
const uint32_t tunings[99][129] = {	{12, 0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000},\ 
								{12, 0, 11100, 20300, 31600, 38600, 49800, 55100, 70200, 81300, 88400, 96800, 108800},\ 
								{12, 0, 18500, 23000, 32500, 40500, 49800, 55100, 70200, 88500, 93000, 102500, 110500},\ 
								{12, 0, 11700, 19300, 31000, 38600, 50300, 57900, 69700, 77300, 89000, 96600, 108300},\ 
								{12, 0, 9000, 19200, 29400, 39000, 49800, 58800, 69600, 79200, 88800, 99600, 109200},\ 
								{12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800},\ 
								{12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800},\ 
								{12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800},\ 
								{12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800},\ 
								{43, 0, 2150, 5327, 8446, 11173, 15063, 16500, 18240, 20391, 23117, 26687, 29413, 31564, 34740, 38631, 41750, 43508, 47078, 49804, 51955, 55132, 58251, 61749, 64868, 68044, 70196, 72922, 76492, 78249, 81369, 85259, 88436, 90586, 93313, 96883, 99609, 101759, 103499, 104946, 108826, 111553, 114673, 117849},\ 
								{5, 0, 20000, 40000, 70000, 90000}	}; 
									
									//not sure if this will take up too much space, seems OK for now
uint16_t tuningDACTable[128];


void loadTuning(void)
{
	computeTuningDACTable();
}

void computeTuningDACTable(void)
{
	for(int i = 0; i < 128; i++)
	{
		tuningDACTable[i] = calculateDACvalue(i);
	}
}

unsigned short calculateDACvalue(uint8_t noteVal)
{
	uint32_t pitchclass;
	uint32_t templongnote = 0;
	uint32_t octavenum;
	uint32_t templongoctave;
	uint32_t DACval;
	uint32_t note;
	uint32_t cardinality = tunings[tuning][0];
	//templongnote = (noteVal * 54612);  // original simple equal temperament
	
	pitchclass = (noteVal % cardinality);  //get the pitch class
	octavenum = noteVal / cardinality;  //get which octave it occurs in
	
	templongnote = ((tunings[tuning][pitchclass + 1]  / 10) * scaledSemitoneDACvalue);
	
	templongnote = (templongnote / 100000);
	templongoctave = (octavenum * scaledOctaveDACvalue);
	templongoctave = (templongoctave / 100);
	DACval = templongnote + templongoctave;
	if (DACval > 65535)
	{
		DACval = 65535;
	}
	return (uint16_t)DACval;
}