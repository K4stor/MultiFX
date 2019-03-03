#include <Arduino.h>
#include <ApplicationModel.h>
#include "Io.h"

Preset currentPreset;
byte midiMap[32];
byte currentPresetNumber = 0;
byte presetEncoderValue = 0;
byte param1EncoderValue = 0;
byte param2EncoderValue = 0;
byte param3EncoderValue = 0;
byte currentMidiMappingIndex = 0;

void Preset::saveTo(byte index) {
  writePresetData(currentPreset, index);
}

void Preset::loadFrom(byte index) {
  readPresetData(index);
}

void saveMidiMap() {
  writeMidiMapping();
}

void restoreMidiMap() {

}
