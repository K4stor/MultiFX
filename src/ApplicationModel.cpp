#include <Arduino.h>
#include <ApplicationModel.h>

Preset currentPreset;
byte midiMap[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
byte currentPresetNumber = 0;
byte presetEncoderValue = 0;
byte param1EncoderValue = 0;
byte param2EncoderValue = 0;
byte param3EncoderValue = 0;
byte currentMidiMappingIndex = 0;

void Preset::saveTo(byte index) {

}

void Preset::loadFrom(byte index) {

}

void saveMidiMap() {

}

void restoreMidiMap() {

}
