#pragma once

#include <Arduino.h>
#include <SD.h>
#include <string>
#include <vector>

namespace tap {

/*
 *
 * This class manages the SD card and the wav files on it.
 *
 */
class AudioFileManager {
public:
  AudioFileManager() = default;

  void begin(String directory, int csPin = -1) {

    sdCSPin = csPin;

    initSDCard();

    mDirectory = directory;
    populateFilenames();

    Serial.println("AudioFileManager initialized");

    Serial.println("Audio files found:");
    for (const auto &filename : mFilenames) {
      Serial.println(filename);
    }

    Serial.println("Total: " + String(mFilenames.size()));
  }

  const auto &getFilepaths() const { return mFilenames; }

  int numAudioFiles() const { return mFilenames.size(); }

  auto getFilePath(int index) const { return mFilenames.at(index); }

private:
  auto isHiddenFile(const String &filename) { return filename.startsWith("."); }

  auto isWavFile(const String &filename) {
    int m = filename.lastIndexOf(".WAV");
    int a = filename.lastIndexOf(".wav");

    return (m > 0 || a > 0) && !isHiddenFile(filename);
  }

  // Iterate through the directory and populate the filenames,
  // only add the ones that are .wav files
  // This is a simple implementation, it does not handle subdirectories.
  void populateFilenames() {
    File dir = SD.open(mDirectory.c_str());
    while (true) {
      File file = dir.openNextFile();
      if (!file) {
        break;
      }

      // If mDirectory does not end with a slash, add one
      if (mDirectory[mDirectory.length() - 1] != '/') {
        mDirectory += "/";
      }
      String curfile = file.name();

      if (isWavFile(curfile)) {
        mFilenames.push_back((mDirectory + curfile).c_str());
      }

      file.close();
    }
    dir.close();
  }

  void initSDCard() {
    // If sdCSPin is -1, just initialize with default
    if (sdCSPin < 0) {
      if (!SD.begin()) {
        // stop here if no SD card, but print a message
        while (1) {
          Serial.println("Unable to access the SD card");
          delay(500);
        }
      }
    } else {
      if (!SD.begin(sdCSPin)) {
        // stop here if no SD card, but print a message
        while (1) {
          Serial.println("Unable to access the SD card");
          delay(500);
        }
      }
    }
  }

  int sdCSPin = -1;

  String mDirectory;
  std::vector<String> mFilenames;
};

} // namespace tap
