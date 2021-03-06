/*
 MantaMate Version 1.0
 by Jeff Snyder, Snyderphonics
 Contributions by Mike Mulshine and Joshua Becker
*/

#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdint.h>
#include "utilities.h"
#include "memory_spi.h"
#include "7Segment.h"
#include "manta_keys.h"
#include "sequencer_process.h"
#include "direct.h"
#include "usb_protocol_cdc.h"
#include "conf_usb_host.h"
#include "ui.h"
#include "no_device.h"
#include "midi.h"
#include "tuning.h"
#include "no_device.h"
#include "wdt.h"


#define NUM_BYTES_PER_KEYBOARD (NUM_BYTES_PER_HEXMAP+5)
#define NUM_BYTES_PER_MIDIKEYBOARD 263
#define NUM_BYTES_PER_COMPOSITION_BANK  (NUM_BYTES_PER_SEQUENCER*14) //there are now 14 possible sequence slots in composition mode
#define NUM_BYTES_PER_NODEVICE_PATTERN ((32 * 2) * 12)

#define NUM_INST 2
#define NUM_COMP 14

tNoteStack editStack;
tNoteStack noteOnStack; // all notes on at any point during runtime

uint8_t currentHexUI;

void resetEditStack(void);

typedef enum MantaMateDeviceType
{
	DeviceManta,
	DeviceMidi,
	DeviceComputer,
	DeviceController	
} MantaMateDeviceType;

MantaMateDeviceType currentDevice;

typedef enum MantaInstrumentType
{
	SequencerInstrument,
	KeyboardInstrument,
	DirectInstrument,
	MantaInstrumentTypeNil
}MantaInstrumentType;

typedef struct _tMantaInstrument
{
	tKeyboard	keyboard;
	tSequencer	sequencer;
	tDirect		direct;
	MantaInstrumentType type;
} tMantaInstrument;

GlobalDisplayState displayState;

tKeyboard* hexmapEditKeyboard;
MantaInstrument hexmapEditInstrument;
int currentHexmapEditPitch;
int currentHexmapEditHex;
int lastHexmapEditHex;
BOOL hexmapEditMode;

tDirect* editDirect;
MantaInstrument editDirectInstrument;
BOOL directEditMode;
int lastDirectEditHex;
int currentDirectEditHex;
int currentDirectEditOutput;

tSequencer* compositionSequencer;
MantaInstrument compositionSequencerInstrument;
BOOL compositionMode;
int lastCompositionHex;
int currentCompositionHex;
int currentCompositionNumber;

int currentHexmapSelect;
int currentDirectSelect;
int currentSequencerSelect;

MantaInstrument whichCompositionInstrument;
int whichCompositionHex;

DPadStyleType dPadStyle;
uint8_t axesOffset;
uint8_t dPadOffset;
BOOL joystickTriggers;
BOOL joystickIgnoreAxes;

BOOL MidiDeviceFound;
// Encode this in preset
MantaInstrument currentInstrument;

extern uint16_t globalCVGlide;
extern uint16_t globalPitchGlide;

MantaInstrumentType takeoverType;
BOOL takeover;

tKeyboard fullKeyboard;
tDirect fullDirect;
tMIDIKeyboard MIDIKeyboard;
tTuningTable myGlobalTuningTable;
tNoDevicePattern noDevicePatterns;

int prevSentPitch;

wdt_opt_t myWDT;

bool compositionMap[2][NUM_COMP];
int currentComp[2];
	
int currentHexmapHex;

int currentTuningHex;

int subtleInterval;

int mantaCompositionSavePending;
int mantaCompositionLoadPending;

int MPE_mode;

#define NUM_BYTES_PER_PAGE 256
#define NUM_PAGES_PER_SECTOR 16
#define NUM_SECTORS_PER_BLOCK 16
#define NUM_BYTES_PER_SECTOR (NUM_PAGES_PER_SECTOR*NUM_BYTES_PER_PAGE)
#define NUM_BYTES_PER_BLOCK (NUM_SECTORS_PER_BLOCK*NUM_BYTES_PER_SECTOR)

#define NUM_PAGES_PER_MANTA_PRESET 11//((NUM_BYTES_PER_PRESET / 256) + 1) 
#define NUM_BYTES_PER_MANTA_PRESET (NUM_PAGES_PER_MANTA_PRESET * NUM_BYTES_PER_PAGE) //(11 + 256 + 4 + 31 + (3*NUM_BYTES_PER_KEYBOARD) + (3*NUM_BYTES_PER_DIRECT) + (2*NUM_BYTES_PER_SEQUENCER))
#define NUM_SECTORS_PER_MANTA_PRESET 1//((NUM_PAGES_PER_PRESET / 16) + 1)
#define NUM_BYTES_PER_COMPOSITION_BANK_ROUNDED_UP (34*256) //there are now 14 possible sequence slots in composition mode, each one is 615 bytes and there are two sets of 14
#define NUM_PAGES_PER_COMPOSITION_BANK 34
#define NUM_SECTORS_PER_COMPOSITION_BANK 3
#define NUM_SECTORS_BETWEEN_MANTA_PRESETS 16 // leave a bunch of space for manta presets by aligning the presets to block edges

#define NUM_BYTES_PER_MIDI_PRESET (3*256) // 263(num bytes per midi preset besides midikeyboard) + 263(num bytes per midikeyboard) rounded up to multiple of 256 bytes
#define NUM_PAGES_PER_MIDI_PRESET 3//((NUM_BYTES_PER_PRESET / 256) + 1) 
#define NUM_SECTORS_PER_MIDI_PRESET 1//((NUM_PAGES_PER_PRESET / 16) + 1) 

#define NUM_BYTES_PER_NODEVICE_PRESET (2*256) // 3 + NUM_BYTES_PER_NODEVICE_PATTERN (32 steps, 2 bytes per step, * 12 outputs) rounded up to multiple of 256 bytes
#define NUM_PAGES_PER_NODEVICE_PRESET 2//((NUM_BYTES_PER_PRESET / 256) + 1)
#define NUM_SECTORS_PER_NODEVICE_PRESET 1//((NUM_PAGES_PER_PRESET / 16) + 1)

#define NUM_PAGES_PER_TUNING 3
#define NUM_PAGES_PER_HEXMAP 1
#define NUM_PAGES_PER_DIRECT 1
#define NUM_PAGES_PER_SEQUENCER 3
#define NUM_PAGES_PER_STARTUP_STATE 1

#define MANTA_PRESET_STARTING_SECTOR 0
#define MIDI_PRESET_STARTING_SECTOR 1600
#define TUNING_STARTING_SECTOR 1700
#define SEQUENCER_STARTING_SECTOR 1800
#define HEXMAP_STARTING_SECTOR 1900
#define DIRECT_STARTING_SECTOR 1940
#define NODEVICE_PRESET_STARTING_SECTOR 1980
#define STARTUP_STATE_SECTOR 2020


uint8_t mantamate_internal_preset_buffer[NUM_BYTES_PER_MANTA_PRESET]; //was 19456 //now 2560

#define GLOBALS_BYTE_ADDRESS 0

#define KEYBOARD1_BYTE_ADDRESS (11 + 256 + 4 + 31)
#define KEYBOARD2_BYTE_ADDRESS (KEYBOARD1_BYTE_ADDRESS+NUM_BYTES_PER_KEYBOARD)
#define FULLKEYBOARD_BYTE_ADDRESS (KEYBOARD2_BYTE_ADDRESS+NUM_BYTES_PER_KEYBOARD)

#define DIRECT1_BYTE_ADDRESS (FULLKEYBOARD_BYTE_ADDRESS + NUM_BYTES_PER_KEYBOARD)
#define DIRECT2_BYTE_ADDRESS (DIRECT1_BYTE_ADDRESS + NUM_BYTES_PER_DIRECT)
#define FULLDIRECT_BYTE_ADDRESS (DIRECT2_BYTE_ADDRESS + NUM_BYTES_PER_DIRECT)

#define SEQUENCER1_BYTE_ADDRESS  (FULLDIRECT_BYTE_ADDRESS + NUM_BYTES_PER_DIRECT)
#define SEQUENCER2_BYTE_ADDRESS (SEQUENCER1_BYTE_ADDRESS +  NUM_BYTES_PER_SEQUENCER)

#define COMPOSITIONBANK1_BYTE_ADDRESS (SEQUENCER2_BYTE_ADDRESS + NUM_BYTES_PER_SEQUENCER)
#define COMPOSITIONBANK2_BYTE_ADDRESS (COMPOSITIONBANK1_BYTE_ADDRESS + NUM_BYTES_PER_COMPOSITION_BANK)

extern uint8_t encodeBuffer[NUM_INST][NUM_BYTES_PER_SEQUENCER];
extern uint8_t decodeBuffer[NUM_INST][NUM_BYTES_PER_SEQUENCER];
extern uint8_t memoryInternalCompositionBuffer[NUM_INST][NUM_BYTES_PER_COMPOSITION_BANK_ROUNDED_UP];


tMantaInstrument manta[NUM_INST];
// - - - - - - - - - - - - - - - - - - - - -

void mantaPreset_encode(uint8_t* buffer);
void mantaPreset_decode(uint8_t* buffer);

void midiPreset_encode(uint8_t* buffer);
void midiPreset_decode(uint8_t* buffer);

void noDevicePreset_encode(uint8_t* buffer);
void noDevicePreset_decode(uint8_t* buffer);

// OUTPUT
void dacSendPitchMode	(MantaInstrument, uint8_t step);
void dacSendTriggerMode	(MantaInstrument, uint8_t step);

// LEDs
int pitchToKbdHex(int pitch);
void setSequencerLEDs		(void);
void setKeyboardLEDs		(void);
void setDirectLEDs			(void);

void setPanelSelectLEDs		(void);
void setSliderLEDsFor		(MantaInstrument, int note);
void setKeyboardLEDsFor		(MantaInstrument, int note);
void setOptionLEDs			(void);
void setCompositionLEDs     (void);
void setHexmapLEDs			(void);
void setHexmapConfigureLEDs	(void);
void setDirectOptionLEDs	(void);
void setSequencerLEDsFor	(MantaInstrument);

void setTriggerPanelLEDsFor	(MantaInstrument, TriggerPanel panel);
void resetSliderMode		(void);

void mantaSliderTouchAction(int whichSlider);
void mantaSliderReleaseAction(int whichSlider);

tIRamp out[2][6];

tIRamp pitchBendRamp[16];
tIRamp pressureRamp[16];
tIRamp magicRamp[16];

uint8_t readData;

uint8_t* readDataArray[256];

uint16_t mySerialString[8];

uint32_t currentMantaPresetBufferPosition;

BOOL busyWithUSB;

void blink(void);

// TIMER 
volatile avr32_tc_t *tc1;
volatile avr32_tc_t *tc2;
volatile avr32_tc_t *tc3;

#define TRIGGER_TIMING 4

#define CVPITCH 0
#define CVTRIGGER 1
#define CV1P 2
#define CV2P 3
#define CV3P 4
#define CV4P 5

#define CVKPITCH 0
#define CVKGATE 1
#define CVKVEL 2
#define CVKTRIGGER 3
#define CVKSLIDEROFFSET 1


#define CVMAX 2

#define CV1T  0
#define CV2T  3
#define CVTRIG1 1
#define CVTRIG2 2
#define CVTRIG3 4
#define CVTRIG4 5

#define TC1                 (&AVR32_TC)
#define TC1_CHANNEL         0
#define TC1_IRQ             AVR32_TC_IRQ0
#define TC1_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC1_IRQ_PRIORITY    AVR32_INTC_INT0

#define TC2                 (&AVR32_TC)
#define TC2_CHANNEL         1
#define TC2_IRQ             AVR32_TC_IRQ1
#define TC2_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC2_IRQ_PRIORITY    AVR32_INTC_INT2

#define TC3                 (&AVR32_TC)
#define TC3_CHANNEL         2
#define TC3_IRQ             AVR32_TC_IRQ2
#define TC3_IRQ_GROUP       AVR32_TC_IRQ_GROUP
#define TC3_IRQ_PRIORITY    AVR32_INTC_INT1

void initTimers (void);



//DEBUG CODE
extern uint16_t lengthDB;
extern int slider;

//global variables that everything which includes main.h should be able to see
extern uint32_t dummycounter;
extern uint8_t manta_mapper;
extern uint8_t tuning_count;
extern uint8_t new_manta_attached;

extern unsigned char preset_num;
extern unsigned char preset_to_save_num;
extern unsigned char savingActive;
extern uint32_t clock_speed;
extern ConnectedDeviceType type_of_device_connected;
extern SuspendRetrieveType suspendRetrieve;
extern unsigned char number_for_7Seg;
extern unsigned char blank7Seg;
extern unsigned char transpose_indication_active;
extern unsigned char normal_7seg_number;


extern BOOL no_device_mode_active;


uint8_t freeze_LED_update;
uint8_t roll_LEDs;

extern uint32_t upHeld;
extern uint32_t downHeld;
extern uint32_t holdTimeThresh;

int defaultTuningMap[8];

unsigned char tuningLoading;

TuningOrLearnType tuningOrLearn;
uint8_t currentNumberToMIDILearn;

// UI
void touchKeyboardHex(int hex, uint8_t weight);
void releaseKeyboardHex(int hex);
void releaseLingeringKeyboardHex(int hex);
void touchFunctionButtonKeys(MantaButton button);
void releaseFunctionButtonKeys(MantaButton button);

void touchLowerHex				(uint8_t hexagon);
void touchLowerHexOptionMode	(uint8_t hexagon);

void releaseLowerHex			(uint8_t hexagon);
void releaseLowerHexOptionMode	(uint8_t hexagon);

void touchUpperHex				(uint8_t hexagon);
void touchUpperHexOptionMode	(uint8_t hexagon);

void releaseUpperHex			(uint8_t hexagon);
void releaseUpperHexOptionMode	(uint8_t hexagon);

void touchTopLeftButton			(void);
void releaseTopLeftButton		(void);
void touchTopRightButton		(void);
void releaseTopRightButton		(void);
void touchBottomLeftButton		(void);
void releaseBottomLeftButton	(void);
void touchBottomRightButton		(void);
void releaseBottomRightButton	(void);

void allUIStepsOff(MantaInstrument inst);
void uiOff(void);


void initMantaLEDState(void);


void setCurrentInstrument(MantaInstrument inst);

void sendDataToOutput(int which, int ramp, uint16_t data);

//set up the external interrupt for the gate input
void setupEIC(void);
void updatePreset(void);
void updatePreferences(void);
void updateSave(void);
void Preset_Switch_Check(uint8_t whichSwitch);
void Save_Switch_Check(void);
void Preferences_Switch_Check(void);
void USB_Mode_Switch_Check(void);
void clockHappened(void);
void enterBootloader(void);
void sendDataToExternalMemory(void);
void savePreset(void);
void loadNoDevicePreset(void);
void loadMIDIPreset(void);
void loadJoystickPreset(void);
void loadMantaPreset(void);
void clearDACoutputs(void);
void clearInstrumentDACoutputs(MantaInstrument inst);

uint8_t upSwitch(void);
uint8_t downSwitch(void);
uint8_t preferencesSwitch(void);
uint8_t saveSwitch(void);
uint8_t USBSwitch(void);

uint8_t upSwitchRead(void);
uint8_t downSwitchRead(void);
uint8_t saveSwitchRead(void);
uint8_t preferencesSwitchRead(void);
uint8_t USBSwitchRead(void);
void readAllSwitches(void);

//bootloader related values
#define ISP_FORCE_VALUE               ('M' << 24 | 'S' << 16 | 'I' << 8 | 'F')
#define ISP_FORCE_OFFSET              0x1F8
//start bootloader function
void usb_msc_bl_start(void);

/*! \brief Opens the communication port
 * This is called by CDC interface when USB Host enable it.
 *
 * \retval true if cdc startup is successfully done
 */
bool main_midi_enable(void);

/*! \brief Closes the communication port
 * This is called by CDC interface when USB Host disable it.
 */
void main_midi_disable(void);

/*! \brief Manages the leds behaviors
 * Called when a start of frame is received on USB line each 1ms.
 */
void main_sof_action(void);

/*! \brief Enters the application in low power mode
 * Callback called when USB host sets USB line in suspend state
 */
void main_suspend_action(void);

/*! \brief Turn on a led to notify active mode
 * Called when the USB line is resumed from the suspend state
 */
void main_resume_action(void);



//function prototypes//
void dacSetupwait1(void);
void dacSetupwait2(void);
void dacwait1(void);
void dacwait2(void);
void memoryWait(void);
void DACsetup(void);
void dacsend(unsigned char DACvoice, unsigned char DACnum, unsigned short DACval);

void DAC16Send(unsigned char DAC16voice, unsigned short DAC16val);

void lcd_clear_line(uint8_t linenum);
#endif // _MAIN_H_
