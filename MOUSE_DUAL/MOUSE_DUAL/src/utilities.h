/*
 * utilities.h
 *
 * Created: 11/18/2016 11:00:51 AM
 *  Author: Jeff Snyder
 */ 


#ifndef UTILITIES_H_
#define UTILITIES_H_

#include "stdint.h"
// Sequencer patterns to be used in the program
#define NUM_PATTERNS 8
#define NUM_GLOBAL_OPTIONS 4
#define NUM_PANEL_MOVES 2
#define MAX_STEPS 32

typedef enum MantaLEDColor
{
	Off = 0,
	Amber,
	Red,
	RedOff,
	AmberOff,
	AmberOn,
	RedOn,
	BothOn,
	MantaLEDColorNil,
	
	
}	MantaLEDColor;



typedef enum MantaMap
{
	DefaultMap,
	PianoMap,
	HarmonicMap,
	WickiHaydenMap,
	IsomorphicMap,
	FreeMap,
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
	
	

#define NUM_HEXES 48
#define MAX_VOICES 4



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
	JoystickConnected,
	HIDKeyboardConnected
} ConnectedDeviceType;
// Typedef versions of Manta modes.

typedef enum MantaInstrument {
	InstrumentOne = 0,
	InstrumentTwo,
	InstrumentNil
} MantaInstrument;

typedef enum GlobalPreferences
{
	PRESET_SELECT,
	TUNING_AND_LEARN,
	PORTAMENTO_TIME,
	INTERNAL_CLOCK,
	PREFERENCES_COUNT//so that we can ask for the number of values in the enum
}GlobalPreferences;

typedef enum GlobalDisplayState
{
	TuningHexSelect,
	HexmapPitchSelect,
	UpDownSwitchBlock,
	GlobalDisplayStateNil
} GlobalDisplayState;

typedef enum ClockPreferences
{
	BPM,
	CLOCK_DIVIDER
}ClockPreferences;

typedef enum TuningOrLearnType
{
	TUNING_SELECT,
	MIDILEARN
}TuningOrLearnType;


typedef enum MantaEditPlayMode {
	EditMode = 0,
	PlayToggleMode,
	TrigToggleMode,
	MantaEditPlayModeNil
}MantaEditPlayMode;

typedef enum MantaPlayMode {
	ToggleMode = 0,
	ArpMode,
	TouchMode,
	MantaPlayModeNil,
}MantaPlayMode;

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

typedef enum SubShift
{
	SubShiftBottomLeft = 0,
	SubShiftBottomRight,
	SubShiftTopLeft,
	SubShiftTopRight,
	SubShiftNil
} SubShift;

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
MantaPlayMode playMode;

BOOL shiftOption1;
BOOL shiftOption1Lock;

BOOL shiftOption2;
BOOL shiftOption2Lock;

GlobalOptionType full_vs_split;

GlobalOptionType seq1PvT, seq2PvT;

/* Integer version of Ramp */
typedef struct _tIRamp {
	int32_t inc;
	int32_t inv_sr_us;
	int32_t curr,dest;
	int32_t time;
} tIRamp;

uint16_t tIRampTick(tIRamp *r);
int tIRampSetTime(tIRamp *r, int32_t time);
int tIRampSetDest(tIRamp *r, int32_t dest);
int tIRampInit(tIRamp *r, int32_t sr, int32_t time);


#endif /* UTILITIES_H_ */