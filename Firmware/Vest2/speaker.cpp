/**
 * @file speaker.cpp
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Implementation of Speaker Protocols.
 *
 * This file defines all functions necessary for sound.
 * 
 */

#include "speaker.h"

void speaker_init() {
  Serial.println("speaker_init(): attaching speaker PWM");
  ledcAttach(SPEAKER_OUT, SPEAKER_BASE_FREQ, SPEAKER_PWM_RESOLUTION);
  ledcWrite(SPEAKER_OUT, SPEAKER_DUTY_CYCLE_0);
  Serial.println("speaker_init(): ready");
}

void beep(uint16_t frequency, uint16_t duration) {
  ledcWrite(SPEAKER_OUT, SPEAKER_DUTY_CYCLE_50);
  ledcWriteTone(SPEAKER_OUT, frequency);
  delay(duration);
  ledcWrite(SPEAKER_OUT, SPEAKER_DUTY_CYCLE_0);
}

void playStartUpBeeps() {
  beep(LOW_FREQ, BEEP_LONGER_MS);
}

void playConnectedBeeps() {
  beep(MMED_FREQ, BEEP_LONG_MS);
  delay(GAP_LONG_MS);
  beep(HI_FREQ, BEEP_LONGER_MS);
}

void playDisconnectedBeeps() {
  beep(MMED_FREQ, BEEP_LONG_MS);
  delay(GAP_LONG_MS);
  beep(LOW_FREQ, BEEP_LONGER_MS);
}

void playFireBeeps() {
  beep(HIEST_FREQ, BEEP_SHORT_MS);
  beep(HIER_FREQ, BEEP_SHORT_MS);
  beep(HIEST_FREQ, BEEP_SHORT_MS);
}

void playCantFireBeeps() {
  beep(LOWER_FREQ, BEEP_SHORT_MS);
  delay(GAP_SHORT_MS);
  beep(LOWEST_FREQ, BEEP_SHORTER_MS);
}

void playGotHitBeeps() {
  beep(HMED_FREQ, BEEP_LONG_MS);
}

void playLowHealthBeeps() {
  beep(LMED_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LMED_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LMED_FREQ, BEEP_SHORT_MS);
}

void playDeadBeeps() {
  beep(LOWEST_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LOWEST_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LOWEST_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LOWEST_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LOWEST_FREQ, BEEP_SHORT_MS);
}

void playWinningBeeps() {
  beep(HI_FREQ, BEEP_MEDIUM_MS);
  delay(GAP_MEDIUM_MS);
  beep(MMED_FREQ, BEEP_SHORTER_MS);
  delay(GAP_SHORT_MS);
  beep(HIER_FREQ, BEEP_LONG_MS);
}

void playLosingBeeps() {
  beep(LOW_FREQ, BEEP_SHORT_MS);
  delay(GAP_LONG_MS);
  beep(LMED_FREQ, BEEP_SHORT_MS);
}