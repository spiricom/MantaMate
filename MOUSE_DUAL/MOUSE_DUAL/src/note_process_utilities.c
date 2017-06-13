/*
 * note_process_utilities.c
 *
 * Mostly Birl stuff.
 *
 * Created: 3/28/2017 1:53:56 PM
 *  Author: Mike Mulshine
 */ 

#include "note_process.h"

void BirlBreathPosOut(void)
{
	birlBreathPos = ((birlBreathPosHigh << 7) + (birlBreathPosLow1)) * 20.0 ; // * 20 to bring the 12 bit number up to 16 bit... 
	//Write7Seg(birlBreathPos / 32);
	DAC16Send(1, birlBreathPos);
}

void BirlBreathNegOut(void)
{
	DAC16Send(2, birlBreathNeg);
}
		
float_t calculateBirlPitch(void)
{	
		float_t pitch = 1.0;
		
		for(uint8_t i = 0; i < 11; i++){
			//if (finger[i] > FINGERMAX)
			//{
			//	fingerFloat[i] = (float_t)(FINGERMAX - FINGERMIN) * FINGERMAX_SCALE;
			//}
			//if (finger[i] < FINGERMIN)
			//{
			//	fingerFloat[i] = 0.0;
			//}
			//else
			//{
			//	fingerFloat[i] = ((float)(finger[i] - FINGERMIN)) * FINGERMAX_SCALE;
			//}
			//fingerFloat[i] = (float)finger[i] * FINGERMAX_SCALE; //Scale between 0-200 to 0.-1.
			if (finger[i] > 30)
			{
				fingerFloat[i] = 1.0;
			}
			else
			{
				fingerFloat[i] = 0.0;
			}
		}
		/*
		var L1 = a[0];
		var L2 = a[1];
		var L3 = a[2];
		var LP1 = a[3];
		var LP2 = a[4];
		var Side = a[5];
		var R1 = a[6];
		var R2 = a[7];
		var R3 = a[8];
		var RP1 = a[9];
		var RP2 = a[10];
		*/

		// L1 drops the pitch a whole step (like from C# to B)
		pitch = pitch - (2.0 * fingerFloat[0]);

		// L2 drops the pitch a half step if L1 is open, or a whole step if L1 is closed (C natural or A)
		pitch = pitch - (fingerFloat[1] + (fingerFloat[1] * fingerFloat[0]));

		// L3 drops the pitch a whole step (A to G)
		pitch = pitch - (2.0 * fingerFloat[2]);

		// R1 should drop a whole step if L2 is down, otherwise a half step (for G to F, or B to Bb)
		pitch = (pitch - (fingerFloat[6] + (fingerFloat[6] * fingerFloat[1])));

		// R2 always drops the pitch a half step (for F to E and G to F#)
		pitch = pitch - fingerFloat[7];

		// R3 should drop whole step if R2 is down (moving from E to D), but only a half step otherwise, for flute F#
		pitch = (pitch - (fingerFloat[8] + (fingerFloat[8] * fingerFloat[7])));

		pitch = (pitch + fingerFloat[3]); // Pinky key 1 goes half step up (G#, C#, Eb)  left
		pitch = (pitch + fingerFloat[9]); // Pinky key 1 goes half step up (G#, C#, Eb)  right

		pitch = (pitch - (2.0 * fingerFloat[4])); // Pinky key 2 goes whole step down (C)  left
		pitch = (pitch - (2.0 * fingerFloat[10])); // Pinky key 2 goes whole step down (C)  right

		//pitch = (pitch + (1.0 * Side)); // side key raises any pitch a whole step

		return (pitch + birlOffset + birlOctave);
		//("Final pitch:");


}

void birlPitchOut(void)
{
	float_t tempPitch = 0;
	uint32_t tempDAC = 0;
	
	tempPitch = calculateBirlPitch();
	
	tempDAC = (uint32_t)(tempPitch * 54780.0);
	
	DAC16Send(0, ((uint16_t)(tempDAC / 100)));
}

//For the MIDI keyboards knobs
void controlChangeBirl(uint8_t ctrlNum, uint8_t val)
{
	switch (ctrlNum)
	{
		
		case 30:
		birlBreathPosHigh = val;
		BirlBreathPosOut();
		break;
		case 31:
		birlBreathPosLow1 = val;
		BirlBreathPosOut();
		break;
		case 32:
		birlBreathNegHigh = val;
		BirlBreathNegOut();
		break;
		case 33:
		birlBreathNegLow1 = val;
		BirlBreathNegOut();
		break;
		case 6:
		
		break;
		case 7:
		sysVol = val;
		break;
		case 8:
		
		break;
		
		// birl touch sensors
		case 42:
		finger_highbytes[0] = val;
		finger[0] = (finger_highbytes[0] << 7) + finger_lowbytes[0];
		birlPitchOut();
		break;
		
		case 43:
		finger_lowbytes[0] = val;
		finger[0] = (finger_highbytes[0] << 7) + finger_lowbytes[0];
		birlPitchOut();
		if (finger[0] > 20)
		{
			LED_On(LED3);
		}
		else
		{
			LED_Off(LED3);
		}
		break;
		
		case 44:
		finger_highbytes[1] = val;
		finger[1] = (finger_highbytes[1] << 7) + finger_lowbytes[1];
		birlPitchOut();
		break;
		
		case 45:
		finger_lowbytes[1] = val;
		finger[1] = (finger_highbytes[1] << 7) + finger_lowbytes[1];
		birlPitchOut();
		if (finger[1] > 20)
		{
			LED_On(LED2);
		}
		else
		{
			LED_Off(LED2);
		}
		break;
		
		case 46:
		finger_highbytes[2] = val;
		finger[2] = (finger_highbytes[2] << 7) + finger_lowbytes[2];
		birlPitchOut();
		break;
		
		case 47:
		finger_lowbytes[2] = val;
		finger[2] = (finger_highbytes[2] << 7) + finger_lowbytes[2];
		birlPitchOut();
		break;
		
		case 48:
		finger_highbytes[3] = val;
		finger[3] = (finger_highbytes[3] << 7) + finger_lowbytes[3];
		birlPitchOut();
		break;
		
		case 49:
		finger_lowbytes[3] = val;
		finger[3] = (finger_highbytes[3] << 7) + finger_lowbytes[3];
		birlPitchOut();
		break;
		
		case 50:
		finger_highbytes[4] = val;
		finger[4] = (finger_highbytes[4] << 7) + finger_lowbytes[4];
		birlPitchOut();
		break;
		
		case 51:
		finger_lowbytes[4] = val;
		finger[4] = (finger_highbytes[4] << 7) + finger_lowbytes[4];
		birlPitchOut();
		break;
		
		case 52:
		finger_highbytes[5] = val;
		finger[5] = (finger_highbytes[5] << 7) + finger_lowbytes[5];
		birlPitchOut();
		break;
		
		case 53:
		finger_lowbytes[5] = val;
		finger[5] = (finger_highbytes[5] << 7) + finger_lowbytes[5];
		birlPitchOut();
		break;
		
		case 54:
		finger_highbytes[6] = val;
		finger[6] = (finger_highbytes[6] << 7) + finger_lowbytes[6];
		birlPitchOut();
		break;
		
		case 55:
		finger_lowbytes[6] = val;
		finger[6] = (finger_highbytes[6] << 7) + finger_lowbytes[6];
		birlPitchOut();
		break;
		
		case 56:
		finger_highbytes[7] = val;
		finger[7] = (finger_highbytes[7] << 7) + finger_lowbytes[7];
		birlPitchOut();
		break;
		
		case 57:
		finger_lowbytes[7] = val;
		finger[7] = (finger_highbytes[7] << 7) + finger_lowbytes[7];
		birlPitchOut();
		break;
		
		case 58:
		finger_highbytes[8] = val;
		finger[8] = (finger_highbytes[8] << 7) + finger_lowbytes[8];
		birlPitchOut();
		break;
		
		case 59:
		finger_lowbytes[8] = val;
		finger[8] = (finger_highbytes[8] << 7) + finger_lowbytes[8];
		birlPitchOut();
		break;
		
		case 60:
		finger_highbytes[9] = val;
		finger[9] = (finger_highbytes[9] << 7) + finger_lowbytes[9];
		birlPitchOut();
		break;
		
		case 61:
		finger_lowbytes[9] = val;
		finger[9] = (finger_highbytes[9] << 7) + finger_lowbytes[9];
		birlPitchOut();
		break;
		
		case 62:
		finger_highbytes[10] = val;
		finger[10] = (finger_highbytes[10] << 7) + finger_lowbytes[10];
		birlPitchOut();
		break;
		
		case 63:
		finger_lowbytes[10] = val;
		finger[10] = (finger_highbytes[10] << 7) + finger_lowbytes[10];
		birlPitchOut();
		break;
		
		//octave down switch
		case 70:
		if (val)
		{
			birlOctave = 0;
			birlPitchOut();
			LED_On(LED0);
		}
		else
		{
			birlOctave = 12;
			birlPitchOut();
			LED_Off(LED0);
		}
		break;
		
		//octave up switch
		case 72:
		if (val)
		{
			birlOctave = 24;
			birlPitchOut();
			LED_On(LED1);
		}
		else
		{
			birlOctave = 12;
			birlPitchOut();
			LED_Off(LED1);
		}
		break;
		
		default:
		break;
	}
}
