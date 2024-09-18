/*
 *
 * This simple example demonstrates how to play audio files from a folder in a
 * random order.
 *
 * It works with any of the I2S dacs supported by the Teensy Audio Library
 * (tested with the Teensy Audio Shield (SGTL5000 chip) and PCM5102A).
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
  AudioMemory(10);

  // Open Serial
  Serial.begin(115200);
  while (!Serial || millis() < 1000)
    ;

  // Initialize audio file manager
  audioFileManager.begin(audioDirectory);
  audioFilePlayer.begin();

  // Randomize play order
  audioFilePlayer.shuffle(true);

  // Play first file
  audioFilePlayer.play();

  Serial.println("Setup done");
}

void loop() { audioFilePlayer.update(); }
