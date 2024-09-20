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
 * It includes a volume control and access to the latest peak values of the
 * audio being played. This can be used to set a visualizer or an LED or
 * something to indicate status.
 *
 */
class AudioFilePlayer {
public:
  explicit AudioFilePlayer(AudioFileManager &manager)
      : mAudioFileManager(manager) {}

  bool fileFinished() {
    const auto position = playSd1.positionMillis();
    const auto length = playSd1.lengthMillis();

    return position >= length;
  }

  // Returns a value between 0.0 and 1.0
  float progress() {
    const auto position = playSd1.positionMillis();
    const auto length = playSd1.lengthMillis();

    return static_cast<float>(position) / static_cast<float>(length);
  }

  bool begin() {
    AudioNoInterrupts();
    setupAudioConnections();

#ifdef USING_TEENSY_AUDIO_SHIELD
    audioShield.enable();
    audioShield.inputSelect(AUDIO_INPUT_LINEIN);
    audioShield.volume(1.0);
#endif

    // Set the volume
    setVolume(0.5f);

    playSd1.begin();

    AudioInterrupts();

    return true;
  }

  void update() {
    // Update the audio player
    // TODO:
    // - Gap between audio files

    if (mIsPlaying) {
      const auto done = fileFinished();

      // If file finished, do something
      if (done) {

        Serial.println("File finished");

        // Play next
        if (mShuffle) {
          Serial.println("Shuffle mode active, randomizing next file");
          randomize();
        } else {
          Serial.println("Playing next file");
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
    playSd1.stop();
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
      play();
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
      play();
    }
  }

  void randomize() {
    auto newIndex = random(0, mAudioFileManager.numAudioFiles());

    while (newIndex == mCurrentPlayingFileIndex) {
      newIndex = random(0, mAudioFileManager.numAudioFiles());
    }

    Serial.println("Randomizing to " + String(newIndex));
    mCurrentPlayingFileIndex = newIndex;

    play();
  }

  bool isPlaying() { return playSd1.isPlaying(); }

  // Enable or disable shuffle mode
  void shuffle(bool enable) { mShuffle = enable; }

  void toggleShuffle() { mShuffle = !mShuffle; }

  // A number between 0.0 and 1.0
  void setVolume(float volume) {
    AudioNoInterrupts();
    amp_left.gain(volume);
    amp_right.gain(volume);
    AudioInterrupts();
  }

  void setupAudioConnections() {
    disconnectAll();

    // Direct output
    // patchCord1.connect(playSd1, 0, i2s2, 0);
    // patchCord2.connect(playSd1, 1, i2s2, 1);

    // Amplify
    patchCord5.connect(playSd1, 0, amp_left, 0);
    patchCord6.connect(playSd1, 1, amp_right, 0);

    // Output
    patchCord7.connect(amp_left, 0, i2s2, 0);
    patchCord8.connect(amp_right, 0, i2s2, 1);

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
    const auto ok = playSd1.play(fileName.c_str());
    return ok;
  }

  auto getPeakLeft() { return peak_left.read(); }
  auto getPeakRight() { return peak_right.read(); }

protected:
  bool mShuffle = false;
  bool mIsPlaying = false;

  int mCurrentPlayingFileIndex = 0;

#ifdef USING_TEENSY_AUDIO_SHIELD
  AudioControlSGTL5000 audioShield;
#endif

  AudioFileManager &mAudioFileManager;

  AudioOutputI2S i2s2;
  AudioAnalyzePeak peak_left{}, peak_right{};
  AudioAmplifier amp_left{}, amp_right{};
  AudioPlaySdWav playSd1;
  AudioConnection patchCord1, patchCord2, patchCord3, patchCord4, patchCord5,
      patchCord6, patchCord7, patchCord8;
};

} // namespace tap
