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