/*
 *
 * This simple example demonstrates how to play audio files from a folder in a
 * random order.
 *
 * It works with any of the I2S dacs supported by the Teensy Audio Library
 * (tested with the Teensy Audio Shield (SGTL5000 chip) and PCM5102A).
 *
 * By default it uses the default i2s output, but if you choose to use this with
 * the Teensy Audio Shield, you need to define the USING_TEENSY_AUDIO_SHIELD
 * macro which will turn on the SGTL5000 output.
 *
 */
#include <Arduino.h>

#include <Audio.h>
#include <Bounce.h>
#include <SPI.h>
#include <TeensyAudioFilePlayer/AudioFileManager.h>
#include <TeensyAudioFilePlayer/AudioFilePlayer.h>
#include <Wire.h>

constexpr auto audioDirectory = "/audio";

// Audio file player and sd card manager
tap::AudioFileManager audioFileManager;
tap::AudioFilePlayer audioFilePlayer(audioFileManager);

// Arduino Setup
void setup(void) {
  AudioMemory(16);

  // Open Serial
  Serial.begin(9600);
  // while (!Serial || millis() < 1000) {
  //   delay(1);
  // };

  // Initialize audio file manager
#ifdef USING_TEENSY_AUDIO_SHIELD
#pragma message "Using Teensy Audio Shield"

  const auto csPin = 10;
  constexpr int mosiPin = 7;
  constexpr int sckPin = 14;
  constexpr int misoPin = 12;

  SPI.setMOSI(mosiPin);
  SPI.setSCK(sckPin);
  SPI.setMISO(misoPin);

  audioFileManager.begin(audioDirectory, csPin);
#else
  audioFileManager.begin(audioDirectory);
#endif
  audioFilePlayer.begin();

  audioFilePlayer.setVolume(0.5f);

  // Randomize play order
  audioFilePlayer.shuffle(true);

  // Play first file
  audioFilePlayer.play();

  Serial.println("Setup done");
}

void loop() { audioFilePlayer.update(); }
