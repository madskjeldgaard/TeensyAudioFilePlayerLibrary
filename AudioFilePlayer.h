#pragma once

#include "AudioFileManager.h"
#include <Arduino.h>
#include <Audio.h>
#include <SD.h>
#include <memory>
#include <play_sd_aac.h>
#include <play_sd_flac.h>
#include <play_sd_mp3.h>
#include <play_sd_opus.h>

namespace tap {

class AudioFilePlayer {
public:
  explicit AudioFilePlayer(AudioFileManager &manager)
      : mAudioFileManager(manager) {}

  bool fileFinished() {
    const auto position = playSdWav.positionMillis();
    const auto length = playSdWav.lengthMillis();
    return position >= length;
  }

  float progress() {
    const auto position = playSdWav.positionMillis();
    const auto length = playSdWav.lengthMillis();
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

    setVolume(0.5f);
    AudioInterrupts();
    return true;
  }

  void update() {
    if (mIsPlaying) {
      const auto done = fileFinished();
      if (done) {
        Serial.println("File finished");
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
    auto ok = playAudioFile(path);
    mIsPlaying = ok;
    Serial.println(ok ? "Playing audio file " + path
                      : "Could not play " + path);
  }

  void stop() {
    mIsPlaying = false;
    playSdWav.stop();
  }

  void togglePlay() {
    if (mIsPlaying) {
      stop();
    } else {
      play();
    }
  }

  void next() {
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

  bool isPlaying() { return playSdWav.isPlaying(); }

  void shuffle(bool enable) { mShuffle = enable; }

  void toggleShuffle() { mShuffle = !mShuffle; }

  void setVolume(float volume) {
    AudioNoInterrupts();
    amp_left.gain(volume);
    amp_right.gain(volume);
    AudioInterrupts();
  }

  void setupAudioConnections() {
    // Connect WAV player to mixers
    patchCord1 = AudioConnection(playSdWav, 0, mixerLeft, 0);
    patchCord2 = AudioConnection(playSdWav, 1, mixerRight, 0);

    // Connect MP3 player to mixers
    patchCord3 = AudioConnection(playSdMp3, 0, mixerLeft, 1);
    patchCord4 = AudioConnection(playSdMp3, 1, mixerRight, 1);

    // Connect AAC player to mixers
    patchCord5 = AudioConnection(playSdAac, 0, mixerLeft, 2);
    patchCord6 = AudioConnection(playSdAac, 1, mixerRight, 2);

    // Connect FLAC player to mixers
    patchCord7 = AudioConnection(playSdFlac, 0, mixerLeft, 3);
    patchCord8 = AudioConnection(playSdFlac, 1, mixerRight, 3);

    // Connect mixers to amplifiers
    patchCord9 = AudioConnection(mixerLeft, 0, amp_left, 0);
    patchCord10 = AudioConnection(mixerRight, 0, amp_right, 0);

    // Connect amplifiers to output
    patchCord11 = AudioConnection(amp_left, 0, i2s2, 0);
    patchCord12 = AudioConnection(amp_right, 0, i2s2, 1);

    // Analysis
    patchCord13 = AudioConnection(playSdWav, 0, peak_left, 0);
    patchCord14 = AudioConnection(playSdWav, 1, peak_right, 1);
  }

  bool playAudioFile(String fileName) {
    switch (getFileType(fileName)) {
    case SupportedFileTypes::WAV:
      return playSdWav.play(fileName.c_str());
    case SupportedFileTypes::MP3:
      return playSdMp3.play(fileName.c_str());
    case SupportedFileTypes::AAC:
      return playSdAac.play(fileName.c_str());
    case SupportedFileTypes::FLAC:
      return playSdFlac.play(fileName.c_str());
    case SupportedFileTypes::OPUS:
      return playSdOpus.play(fileName.c_str());
    default:
      Serial.println("Unsupported file type");
      return false;
    }
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
  AudioPlaySdWav playSdWav;
  AudioPlaySdMp3 playSdMp3;
  AudioPlaySdAac playSdAac;
  AudioPlaySdFlac playSdFlac;

  // FIXME: Unconnected
  AudioPlaySdOpus playSdOpus;

  AudioMixer4 mixerLeft, mixerRight;

  AudioConnection patchCord1, patchCord2, patchCord3, patchCord4, patchCord5,
      patchCord6, patchCord7, patchCord8, patchCord9, patchCord10, patchCord11,
      patchCord12, patchCord13, patchCord14;
};

} // namespace tap
