/*
 * hex.c
 *
 * Created: 6/26/2017 3:49:13 PM
 *  Author: Mike Mulshine
 */ 


#include "hex.h"

void tHex_init(tHex* const hex, int which)
{
	hex->active = FALSE;
	hex->pitch = which;
	hex->color = Amber;
	
	hex->fine = 2048;
}