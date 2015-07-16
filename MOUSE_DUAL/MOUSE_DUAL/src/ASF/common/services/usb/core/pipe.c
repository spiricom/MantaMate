/*
 * pipe.c
 *
 * Created: 7/16/2015 3:54:02 PM
 *  Author: AirWolf
 */ 

#include "udd.h"
#include "pipe.h"

uint8_t Pipe_WaitUntilReady(void)
{
	#if (USB_STREAM_TIMEOUT_MS < 0xFF)
	uint8_t  TimeoutMSRem = USB_STREAM_TIMEOUT_MS;
	#else
	uint16_t TimeoutMSRem = USB_STREAM_TIMEOUT_MS;
	#endif

	uint16_t PreviousFrameNumber = udd_get_frame_number();

	for (;;)
	{
		if (uhd_get_pipe_token() == PIPE_TOKEN_IN)
		{
			if (Pipe_IsINReceived())
			return PIPE_READYWAIT_NoError;
		}
		else
		{
			if (Pipe_IsOUTReady())
			return PIPE_READYWAIT_NoError;
		}

		if (Pipe_IsStalled())
		return PIPE_READYWAIT_PipeStalled;
		else if (USB_HostState == HOST_STATE_Unattached)
		return PIPE_READYWAIT_DeviceDisconnected;

		uint16_t CurrentFrameNumber = udd_get_frame_number();

		if (CurrentFrameNumber != PreviousFrameNumber)
		{
			PreviousFrameNumber = CurrentFrameNumber;

			if (!(TimeoutMSRem--))
			return PIPE_READYWAIT_Timeout;
		}
	}
}