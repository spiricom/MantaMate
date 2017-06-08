/*
 * pipe.h
 *
 * Created: 7/16/2015 3:53:54 PM
 *  Author: AirWolf
 */ 


#ifndef PIPE_H_
#define PIPE_H_

typedef struct
{
	uint8_t  address; /**< Address of the pipe to configure, or zero if the table entry is to be unused. */
	uint16_t size; /**< Size of the pipe bank, in bytes. */
	uint8_t  epAddress; /**< Address of the endpoint in the connected device. */
	uint8_t  type; /**< Type of the endpoint, a \c EP_TYPE_* mask. */
	uint8_t  banks; /**< Number of hardware banks to use for the pipe. */
} usb_pipe_table_t;

#define ATTR_ALWAYS_INLINE           __attribute__ ((always_inline))
#define ATTR_WARN_UNUSED_RESULT      __attribute__ ((warn_unused_result))
#define PIPE_EPNUM_MASK               0x0F
#define USB_STREAM_TIMEOUT_MS       100

/* Private Interface - For use in library only: */
#if !defined(__DOXYGEN__)
/* Macros: */
#define PIPE_HSB_ADDRESS_SPACE_SIZE     (64 * 1024UL)

/* External Variables: */
extern volatile uint32_t USB_Pipe_SelectedPipe;
extern volatile uint8_t* USB_Pipe_FIFOPos[];
#endif
enum Pipe_WaitUntilReady_ErrorCodes_t
			{
				PIPE_READYWAIT_NoError                 = 0, /**< Pipe ready for next packet, no error. */
				PIPE_READYWAIT_PipeStalled             = 1,	/**< The device stalled the pipe while waiting. */
				PIPE_READYWAIT_DeviceDisconnected      = 2,	/**< Device was disconnected from the host while waiting. */
				PIPE_READYWAIT_Timeout                 = 3, /**< The device failed to accept or send the next packet
				                                             *   within the software timeout period set by the
				                                             *   \ref USB_STREAM_TIMEOUT_MS macro.
				                                             */
			};

static inline void Pipe_SelectPipe(const uint8_t Address) ATTR_ALWAYS_INLINE;
static inline void Pipe_SelectPipe(const uint8_t Address)
{
	USB_Pipe_SelectedPipe = (Address & PIPE_EPNUM_MASK);
}

/** Unfreezes the selected pipe, allowing it to communicate with an attached device. */
static inline void Pipe_Unfreeze(void) ATTR_ALWAYS_INLINE;
static inline void Pipe_Unfreeze(void)
{
	(&AVR32_USBB.UPCON0CLR)[USB_Pipe_SelectedPipe].pfreezec = true;
}

static inline uint16_t Pipe_BytesInPipe(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
static inline uint16_t Pipe_BytesInPipe(void)
{
	return (&AVR32_USBB.UPSTA0)[USB_Pipe_SelectedPipe].pbyct;
}

static inline void Pipe_ClearOUT(void) ATTR_ALWAYS_INLINE;
static inline void Pipe_ClearOUT(void)
{
	(&AVR32_USBB.UPSTA0CLR)[USB_Pipe_SelectedPipe].txoutic  = true;
	(&AVR32_USBB.UPCON0CLR)[USB_Pipe_SelectedPipe].fifoconc = true;
	USB_Pipe_FIFOPos[USB_Pipe_SelectedPipe] = &AVR32_USBB_SLAVE[USB_Pipe_SelectedPipe * PIPE_HSB_ADDRESS_SPACE_SIZE];
}

uint8_t Pipe_WaitUntilReady(void);

static inline void Pipe_Freeze(void) ATTR_ALWAYS_INLINE;
static inline void Pipe_Freeze(void)
{
	(&AVR32_USBB.UPCON0SET)[USB_Pipe_SelectedPipe].pfreezes = true;
}

static inline bool Pipe_IsINReceived(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
static inline bool Pipe_IsINReceived(void)
{
	return (&AVR32_USBB.UPSTA0)[USB_Pipe_SelectedPipe].rxini;
}

static inline bool Pipe_IsOUTReady(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
static inline bool Pipe_IsOUTReady(void)
{
	return (&AVR32_USBB.UPSTA0)[USB_Pipe_SelectedPipe].txouti;
}

static inline bool Pipe_IsStalled(void) ATTR_WARN_UNUSED_RESULT ATTR_ALWAYS_INLINE;
static inline bool Pipe_IsStalled(void)
{
	return (&AVR32_USBB.UPSTA0)[USB_Pipe_SelectedPipe].rxstalldi;
}
#endif /* PIPE_H_ */