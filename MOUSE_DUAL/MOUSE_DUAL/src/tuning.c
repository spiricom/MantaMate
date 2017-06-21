/*
 * CFile1.c
 *
 * Created: 6/21/2017 4:46:56 PM
 *  Author: Jeff Snyder
 */ 

#include "main.h"
#include <asf.h>


uint32_t twelvetet[13] = {12, 0, 10000, 20000, 30000, 40000, 50000, 60000, 70000, 80000, 90000, 100000, 110000};
uint32_t overtonejust[13] = {12, 0, 11100, 20300, 31600, 38600, 49800, 55100, 70200, 81300, 88400, 96800, 108800};
uint32_t kora1[13] = {12, 0, 18500, 23000, 32500, 40500, 49800, 55100, 70200, 88500, 93000, 102500, 110500};
uint32_t meantone[13] = {12, 0, 11700, 19300, 31000, 38600, 50300, 57900, 69700, 77300, 89000, 96600, 108300};
uint32_t werckmeister1[13] = {12, 0, 9000, 19200, 29400, 39000, 49800, 58800, 69600, 79200, 88800, 99600, 109200};
uint32_t werckmeister3[13] = {12, 0, 9600, 20400, 30000, 39600, 50400, 60000, 70200, 79200, 90000, 100200, 109800};
	
uint32_t userTuning[128] = {0};	

//unsigned long numTunings = 6; // we need to think about how to structure this more flexibly. Should maybe be a Tunings struct that includes structs that define the tunings, and then we won't have to manually edit this. Also important for users being able to upload tunings via computer.
