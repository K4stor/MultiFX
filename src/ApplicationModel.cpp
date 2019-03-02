#include <Arduino.h>
#include <ApplicationModel.h>

Preset currentPreset;
byte midiMap[32];
byte currentPresetNumber = 1;
byte presetEncoderValue = 1;
byte param1EncoderValue = 0;
byte param2EncoderValue = 0;
byte param3EncoderValue = 0;
byte currentMidiMappingIndex = 1;

void Preset::saveTo(byte index) {

}

void Preset::loadFrom(byte index) {

}

void saveMidiMap() {

}

void restoreMidiMap() {

}
