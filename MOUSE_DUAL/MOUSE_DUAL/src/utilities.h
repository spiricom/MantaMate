/*
 * utilities.h
 *
 * Created: 11/18/2016 11:00:51 AM
 *  Author: Jeff Snyder
 */ 


#ifndef UTILITIES_H_
#define UTILITIES_H_

// Sequencer patterns to be used in the program
#define NUM_PATTERNS 8
#define NUM_GLOBAL_OPTIONS 4
#define NUM_PANEL_MOVES 2
#define MAX_STEPS 32

typedef enum MantaMap
{
	WickiHaydenMap,
	HarmonicMap,
	PianoMap,
	MantaMapNil
} MantaMap;

typedef enum MantaTuning
{
	TwelveTetTuning,
	OvertoneJustTuning,
	Kora1Tuning,
	MeantoneTuning,
	Werckmeister1Tuning,
	Werckmeister3Tuning,
	GeorgianTuning,
	HighlandBagpipeTuning,
	YoungLMPianoTuning,
	Partch43Tuning,
	LoadedUserTuning,
	MantaTuningNil
} MantaTuning;

typedef enum BOOL 
{
	FALSE = 0,
	TRUE = 1,
	LO = FALSE,
	HI = TRUE
	}BOOL;

#include "stdint.h"

//------------------  S T R U C T U R E S  -------------------
typedef enum GlobalOptionType
{
	FullMode = 0,
	SplitMode,
	PitchMode,
	TriggerMode
} GlobalOptionType;
// Typedef versions of Manta modes.


typedef enum ConnectedDeviceType
{
	NoDeviceConnected = 0,
	MantaConnected,
	MIDIKeyboardConnected,
	MIDIComputerConnected,
	JoystickConnected
} ConnectedDeviceType;
// Typedef versions of Manta modes.

typedef enum MantaInstrument {
	InstrumentOne = 0,
	InstrumentTwo,
	InstrumentFull,
	InstrumentNil
} MantaInstrument;

typedef enum GlobalPreferences
{
	PRESET_SELECT,
	TUNING_SELECT,
	PORTAMENTO_TIME,
	INTERNAL_CLOCK,
	PREFERENCES_COUNT//so that we can ask for the number of values in the enum
}GlobalPreferences;

typedef enum ClockPreferences
{
	BPM,
	CLOCK_DIVIDER
}ClockPreferences;


typedef enum MantaEditPlayMode {
	EditMode = 0,
	PlayToggleMode,
	TrigToggleMode,
	MantaEditPlayModeNil
}MantaEditPlayMode;

typedef enum MantaPlaySubMode {
	SeqMode = 0,
	ArpMode,
	MantaPlayModeNil,
}MantaPlaySubMode;

typedef enum MantaKeySelectMode {
	KeyMode = 0,
	SelectMode,
	MantaKeySelectModeNil
}MantaKeySelectMode;

typedef enum MantaSliderMode {
	SliderModeOne = 0, //CV1, CV2
	SliderModeTwo,     //CV3, CV4
	SliderModeThree,   //OCTAVE, STEPLENGTH
	SliderModePitch,
	SliderModeGlide,
	SliderModeNil
}MantaSliderMode;

typedef enum MantaSlider {
	SliderOne = 0,
	SliderTwo,
	SliderNil
}MantaSlider;

typedef enum MantaButton {
	ButtonTopLeft = 0,
	ButtonTopRight = 1,
	ButtonBottomLeft = 2,
	ButtonBottomRight = 3,
	SliderModeButton = ButtonTopLeft,
	OptionModeButton = ButtonBottomLeft,
	EditToggleButton = ButtonTopRight,
	PlayModeButton2 = ButtonBottomRight,
	ButtonNil
}MantaButton;

typedef enum KeyboardOptionMode
{
	KeyboardMode,
	OptionMode,
	KeyboardOptionModeNil,
}KeyboardOptionMode;

typedef enum 
Action
{
	CompositionWrite,
	CompositionRead,
	CompositionCopy,
	CompositionActionNil
} CompositionAction;

typedef enum PanelSwitch
{
	PanelLeft,
	PanelRight,
	PanelSwitchNil,
} PanelSwitch;



typedef enum SequencerPatternType {
	LeftRightRowDown,
	LeftRightRowUp,
	LeftRightDiagDown,
	LeftRightDiagUp,
	LeftRightColDown,
	RightLeftColUp,
	RandomWalk,
	OrderTouch,
	
	//not using atm
	RecordTouch,
	RightLeftRowDown,
	RightLeftRowUp,
	RightLeftDiagDown,
	RightLeftDiagUp,
	LeftRightColUp,
	RightLeftColDown,
	SequencerPatternTypeNil,
	
}SequencerPatternType;

typedef enum TriggerPanel {
	PanelNil = -1,
	PanelOne = 0,
	PanelTwo,
	PanelThree,
	PanelFour,
} TriggerPanel;

typedef enum TuningLoadLocation {
	Local = 0,
	External
} TuningLoadLocation;

typedef enum StepParameterType {
	Toggled = 0,
	Length,
	CV1,
	CV2,
	CV3,
	CV4,
	Pitch,
	Fine,
	Octave,
	Note,
	KbdHex,
	PitchGlide,
	CVGlide,
	On1,
	On2,
	On3,
	On4
}StepParameterType;


// Sequencer Modes
SequencerPatternType pattern_type;


MantaSliderMode prevMantaSliderMode;
MantaSliderMode prevMantaSliderModeForOctaveHexDisable;
MantaSliderMode currentMantaSliderMode;

MantaEditPlayMode edit_vs_play;
MantaButton currentFunctionButton;
MantaPlaySubMode playSubMode;
KeyboardOptionMode key_vs_option;

GlobalOptionType full_vs_split;

GlobalOptionType seq1PvT, seq2PvT;


/* Ramp */
typedef struct _tRamp {
	float inc;
	float inv_sr_ms;
	float curr,dest;
	uint16_t time;
	int samples_per_tick;

} tRamp;

float tRampTick(tRamp *r);
int tRampSetTime(tRamp *r, float time);
int tRampSetDest(tRamp *r, float dest);
int tRampInit(tRamp *r, float sr, uint16_t time, int samples_per_tick);


#endif /* UTILITIES_H_ */