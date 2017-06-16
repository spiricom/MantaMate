/*
 * CFile1.c
 *
 * Created: 10/23/2015 2:07:33 PM
 *  Author: AirWolf
 */
#include "main.h"
#include <asf.h>


void Write7Seg(uint8_t value)
{
	number_for_7Seg = value;
	
	if (!blank7Seg)
	{
		// send 255 to blank the 7-segment display
		if (value == 255)
		{
			gpio_set_gpio_pin(SEG1_GPIO);
			gpio_set_gpio_pin(SEG2_GPIO);
			gpio_set_gpio_pin(SEG3_GPIO);
			gpio_set_gpio_pin(SEG4_GPIO);
			gpio_set_gpio_pin(SEG5_GPIO);
			gpio_set_gpio_pin(SEG6_GPIO);
			gpio_set_gpio_pin(SEG7_GPIO);
			gpio_set_gpio_pin(SEG8_GPIO);
		}
		
		//send a value from 200 to 209 to have the first digit blank and the second one active (0-9)
		else if ((value >=200) && (value <= 209))
		{
			
			//first blank the left digit
			gpio_set_gpio_pin(SEG1_GPIO);
			gpio_set_gpio_pin(SEG2_GPIO);
			gpio_set_gpio_pin(SEG3_GPIO);
			gpio_set_gpio_pin(SEG4_GPIO);
			

			uint8_t Digit = value % 10;
			
			if (Digit & 1)
			{
				gpio_set_gpio_pin(SEG5_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG5_GPIO);
			}
			if (Digit & 2)
			{
				gpio_set_gpio_pin(SEG6_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG6_GPIO);
			}
			if (Digit & 4)
			{
				gpio_set_gpio_pin(SEG7_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG7_GPIO);
			}
			if (Digit & 8)
			{
				gpio_set_gpio_pin(SEG8_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG8_GPIO);
			}
		}
		
		else
		{
			uint8_t Digits[2];
			Digits[1] = value % 10;
			Digits[0] = value / 10;
			if (Digits[0] & 1)
			{
				gpio_set_gpio_pin(SEG1_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG1_GPIO);
			}
			if (Digits[0] & 2)
			{
				gpio_set_gpio_pin(SEG2_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG2_GPIO);
			}
			if (Digits[0] & 4)
			{
				gpio_set_gpio_pin(SEG3_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG3_GPIO);
			}
			if (Digits[0] & 8)
			{
				gpio_set_gpio_pin(SEG4_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG4_GPIO);
			}
			if (Digits[1] & 1)
			{
				gpio_set_gpio_pin(SEG5_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG5_GPIO);
			}
			if (Digits[1] & 2)
			{
				gpio_set_gpio_pin(SEG6_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG6_GPIO);
			}
			if (Digits[1] & 4)
			{
				gpio_set_gpio_pin(SEG7_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG7_GPIO);
			}
			if (Digits[1] & 8)
			{
				gpio_set_gpio_pin(SEG8_GPIO);
			}
			else
			{
				gpio_clr_gpio_pin(SEG8_GPIO);
			}
		}
	}
	
	else
	{
		gpio_set_gpio_pin(SEG1_GPIO);
		gpio_set_gpio_pin(SEG2_GPIO);
		gpio_set_gpio_pin(SEG3_GPIO);
		gpio_set_gpio_pin(SEG4_GPIO);
		gpio_set_gpio_pin(SEG5_GPIO);
		gpio_set_gpio_pin(SEG6_GPIO);
		gpio_set_gpio_pin(SEG7_GPIO);
		gpio_set_gpio_pin(SEG8_GPIO);
	}
	
	
}