#pragma once

#include "AudioFileManager.h"
#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#include <memory>

namespace tap {

/**
 * @class AudioFilePlayer
 * @brief This class plays audio files and keeps track of currently playing
 * index. It's also responsible of managing an audio driver to play the files
 * with.
 *
 */
class AudioFilePlayer {
public:
  explicit AudioFilePlayer(AudioFileManager &manager)
      : mAudioFileManager(manager) {}

  bool begin() {
    AudioNoInterrupts();
    setupAudioConnections();
    AudioInterrupts();
    return true;
  }

  void update() {
    // Update the audio player
    // TODO:
    // - Gap between audio files

    if (mIsPlaying) {
      const auto fileFinished = !isPlaying();

      // TODO: Use for adding gaps between files
      const auto timeOfFileFinished = millis();

      // If file finished, do something
      if (fileFinished) {
        // Play next
        if (mShuffle) {
          randomize();
        } else {
          next();
        }
      }
    }
  }

  void play() {
    const auto path = mAudioFileManager.getFilePath(mCurrentPlayingFileIndex);

    auto ok = playWav(path);

    mIsPlaying = ok;
    Serial.println(ok ? "Playing audio file " + path
                      : "Could not play " + path);
  }

  void stop() {
    mIsPlaying = false;
    // Stop the audio
    AudioNoInterrupts();
    playSd1.stop();
    AudioInterrupts();
  }

  void togglePlay() {
    // Toggle the play state
    if (mIsPlaying) {
      stop();
    } else {
      play();
    }
  }

  void next() {
    // Play the next audio
    Serial.println("Playing next file");
    mCurrentPlayingFileIndex =
        (mCurrentPlayingFileIndex + 1) % mAudioFileManager.numAudioFiles();

    if (mShuffle) {
      randomize();
    } else {
      if (mIsPlaying) {
        stop();
        play();
      }
    }
  }

  void prev() {
    // Wrap around
    if (mCurrentPlayingFileIndex <= 0) {
      mCurrentPlayingFileIndex = mAudioFileManager.numAudioFiles() - 1;
    } else {
      mCurrentPlayingFileIndex =
          (mCurrentPlayingFileIndex - 1) % mAudioFileManager.numAudioFiles();
    }

    if (mShuffle) {
      randomize();
    } else {
      if (mIsPlaying) {
        stop();
        play();
      }
    }
  }

  void randomize() {
    auto newIndex = random(0, mAudioFileManager.numAudioFiles());

    while (newIndex == mCurrentPlayingFileIndex) {
      newIndex = random(0, mAudioFileManager.numAudioFiles());
    }

    Serial.println("Randomizing to " + String(newIndex));
    mCurrentPlayingFileIndex = newIndex;

    if (mIsPlaying) {
      stop();
      play();
    }
  }

  bool isPlaying() { return playSd1.isPlaying(); }

  // Enable or disable shuffle mode
  void shuffle(bool enable) { mShuffle = enable; }

  void toggleShuffle() { mShuffle = !mShuffle; }

  void setupAudioConnections() {
    Serial.println("Setting up audio connections without EQ");
    disconnectAll();

    // Direct output
    patchCord1.connect(playSd1, 0, i2s2, 0);
    patchCord2.connect(playSd1, 1, i2s2, 1);

    // Analysis
    patchCord3.connect(playSd1, 0, peak_left, 0);
    patchCord4.connect(playSd1, 1, peak_right, 1);
  }

  void disconnectAll() {
    patchCord1.disconnect();
    patchCord2.disconnect();
    patchCord3.disconnect();
    patchCord4.disconnect();
    patchCord5.disconnect();
    patchCord6.disconnect();
    patchCord7.disconnect();
    patchCord8.disconnect();
  }

  bool playWav(String fileName) {
    AudioNoInterrupts();
    const auto ok = playSd1.play(fileName.c_str());
    AudioInterrupts();
    return ok;
  }

protected:
  bool mShuffle = false;
  bool mIsPlaying = false;

  int mCurrentPlayingFileIndex = 0;

  AudioFileManager &mAudioFileManager;

  AudioOutputI2S i2s2;
  AudioAnalyzePeak peak_left{}, peak_right{};
  AudioPlaySdWav playSd1;
  AudioConnection patchCord1, patchCord2, patchCord3, patchCord4, patchCord5,
      patchCord6, patchCord7, patchCord8;
};

} // namespace tap
