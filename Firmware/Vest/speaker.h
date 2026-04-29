/**
 * @file speaker.h
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Definition of Speaker Protocols.
 *
 * This file defines all variables, structs, and functions necessary for sound.
 * 
 * @see config.h for the hardware definitions like GPIO mapping
 */

#include <Arduino.h>

#ifndef SPEAKER_H
#define SPEAKER_H

#define SPEAKER_OUT             2                                   /** PWM Output to Speaker */
#define SPEAKER_BASE_FREQ       2000                                /** Frequency of PWM Output*/
#define SPEAKER_PWM_RESOLUTION  8                                   /** Resoultion of PWM Duty Cycle*/
#define SPEAKER_DUTY_CYCLE_50   ((1 << SPEAKER_PWM_RESOLUTION) / 2) /** 50% Duty Cycle based on Resolution */
#define SPEAKER_DUTY_CYCLE_0    0                                   /** 0% Duty Cycle regardless of Resolution */

#define BEEP_SHORTER_MS         40                                  /** Shorter Beep Durations in MS*/
#define BEEP_SHORT_MS           50                                  /** Short Beep Durations in MS*/
#define BEEP_MEDIUM_MS          70                                  /** Medium Beep Durations in MS*/
#define BEEP_LONG_MS            100                                 /** Long Beep Durations in MS*/
#define BEEP_LONGER_MS          200                                 /** Longer Beep Durations in MS*/

#define GAP_SHORT_MS            20                                  /** Short Pause Durations in MS*/
#define GAP_MEDIUM_MS           30                                  /** Medium Pause Durations in MS*/
#define GAP_LONG_MS             50                                  /** Long Pause Durations in MS*/

#define LOWEST_FREQ             200                                 /** Beep Pitches*/
#define LOWER_FREQ              300                                 /** Beep Pitches*/
#define LOW_FREQ                400                                 /** Beep Pitches*/
#define LMED_FREQ               700                                 /** Beep Pitches*/
#define MMED_FREQ               800                                 /** Beep Pitches*/
#define HMED_FREQ               900                                 /** Beep Pitches*/
#define HI_FREQ                 1200                                /** Beep Pitches*/
#define HIER_FREQ               1400                                /** Beep Pitches*/
#define HIEST_FREQ              1700                                /** Beep Pitches*/



/**
 * @brief initializes speaker
 */
void speaker_init();

/**
 * @brief this function beeps for a frequency and time
 * @param frequency of beep
 * @param duration of beep
 */
void beep(uint16_t frequency, uint16_t duration);

/**
 * @brief this function plays beeps when connected
 */
void playConnectedBeeps();

/**
 * @brief this function plays beeps when disconnected
 */
void playDisconnectedBeeps();

/**
 * @brief this function plays beeps when firing
 */
void playFireBeeps();

/**
 * @brief this function plays beeps when you cant fire
 */
void playCantFireBeeps();

/**
 * @brief this function plays beeps when getting hit
 */
void playGotHitBeeps();

/**
 * @brief this function plays beeps when at low health
 */
void playLowHealthBeeps();

/**
 * @brief this function plays beeps when dead
 */
void playDeadBeeps();

/**
 * @brief this function plays beeps when you win
 */
void playWinningBeeps();

/**
 * @brief this function plays beeps when you lose
 */
void playLosingBeeps();

#endif
