/*
 * sequencer_process.h
 *
 * Created: 6/10/2016 1:44:56 PM
 *  Author: Jeff Snyder
 */ 


#ifndef SEQUENCER_PROCESS_H_
#define SEQUENCER_PROCESS_H_

void sequencerStep(void);
void processSequencer(void);
void processSliderSequencer(uint8_t sliderNum, uint16_t val);
void initSequencer(void);
void blinkersOn(void);
void blinkersOff(void);
void blinkersToggle(void);

#endif /* SEQUENCER_PROCESS_H_ */