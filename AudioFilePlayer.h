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

  bool fileFinished() { return progress() >= 1.0f; }

  float progress() {
    switch (mCurrentlyPlayingFileType) {
    case SupportedFileTypes::WAV:
      return static_cast<float>(playSdWav.positionMillis()) /
             static_cast<float>(playSdWav.lengthMillis());
    case SupportedFileTypes::MP3:
      return static_cast<float>(playSdMp3.positionMillis()) /
             static_cast<float>(playSdMp3.lengthMillis());
    case SupportedFileTypes::OPUS:
      return static_cast<float>(playSdOpus.positionMillis()) /
             static_cast<float>(playSdOpus.lengthMillis());
    case SupportedFileTypes::FLAC:
      return static_cast<float>(playSdFlac.positionMillis()) /
             static_cast<float>(playSdFlac.lengthMillis());
    case SupportedFileTypes::AAC:
      return static_cast<float>(playSdAac.positionMillis()) /
             static_cast<float>(playSdAac.lengthMillis());
    case SupportedFileTypes::UNKNOWN:
      return 0.0f;
    }
  }

  float duration() {
    switch (mCurrentlyPlayingFileType) {
    case SupportedFileTypes::WAV:
      return static_cast<float>(playSdWav.lengthMillis());
    case SupportedFileTypes::MP3:
      return static_cast<float>(playSdMp3.lengthMillis());
    case SupportedFileTypes::OPUS:
      return static_cast<float>(playSdOpus.lengthMillis());
    case SupportedFileTypes::FLAC:
      return static_cast<float>(playSdFlac.lengthMillis());
    case SupportedFileTypes::AAC:
      return static_cast<float>(playSdAac.lengthMillis());
    case SupportedFileTypes::UNKNOWN:
      return 0.0f;
    }
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

    // playSdWav.begin();
    // playSdMp3.begin();
    // playSdAac.begin();
    // playSdFlac.begin();
    // playSdOpus.begin();

    // Set all mixer channels to 1
    mixerLeft.gain(0, 1);
    mixerLeft.gain(1, 1);
    mixerLeft.gain(2, 1);
    mixerLeft.gain(3, 1);

    mixerRight.gain(0, 1);
    mixerRight.gain(1, 1);
    mixerRight.gain(2, 1);
    mixerRight.gain(3, 1);

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
    if (mAudioFileManager.numAudioFiles() == 0) {
      Serial.println("No audio files found");
      return;
    }

    stop();

    const auto path = mAudioFileManager.getFilePath(mCurrentPlayingFileIndex);
    auto ok = playAudioFile(path);
    mIsPlaying = ok;
    Serial.println(ok ? "Playing audio file " + path
                      : "Could not play " + path);
  }

  void stop() {
    mIsPlaying = false;

    playSdWav.stop();
    playSdMp3.stop();
    playSdAac.stop();
    playSdFlac.stop();
    playSdOpus.stop();
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
    patchCord1.connect(playSdWav, 0, mixerLeft, 0);
    patchCord2.connect(playSdWav, 1, mixerRight, 0);

    // Connect MP3 player to mixers
    patchCord3.connect(playSdMp3, 0, mixerLeft, 1);
    patchCord4.connect(playSdMp3, 1, mixerRight, 1);

    // Connect AAC player to mixers
    patchCord5.connect(playSdAac, 0, mixerLeft, 2);
    patchCord6.connect(playSdAac, 1, mixerRight, 2);

    // Connect FLAC player to mixers
    patchCord7.connect(playSdFlac, 0, mixerLeft, 3);
    patchCord8.connect(playSdFlac, 1, mixerRight, 3);

    // Connect mixers to amplifiers
    patchCord9.connect(mixerLeft, 0, amp_left, 0);
    patchCord10.connect(mixerRight, 0, amp_right, 0);

    // Connect amplifiers to output
    patchCord11.connect(amp_left, 0, i2s2, 0);
    patchCord12.connect(amp_right, 0, i2s2, 1);

    // Analysis
    patchCord13.connect(amp_left, 0, peak_left, 0);
    patchCord14.connect(amp_right, 1, peak_right, 1);
  }

  bool playAudioFile(String fileName) {
    const auto path = fileName.c_str();
    auto result = false;
    const auto filetype = getFileType(fileName);

    switch (filetype) {
    case SupportedFileTypes::WAV:
      result = playSdWav.play(path);
      break;
    case SupportedFileTypes::MP3:
      result = playSdMp3.play(path);
      break;
    case SupportedFileTypes::AAC:
      result = playSdAac.play(path);
      break;
    case SupportedFileTypes::FLAC:
      result = playSdFlac.play(path);
      break;
    case SupportedFileTypes::OPUS:
      result = playSdOpus.play(path);
      Serial.println("WARNING: Playing OPUS files is not supported yet");
      break;
    default:
      Serial.println("Unsupported file type");
      result = false;
      break;
    }

    mCurrentlyPlayingFileType = filetype;

    return result;
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

  SupportedFileTypes mCurrentlyPlayingFileType = SupportedFileTypes::UNKNOWN;

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
