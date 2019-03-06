#ifndef APPLICATION_MODEL_H
#define APPLICATION_MODEL_H

#include <Arduino.h>

struct Preset {
  byte param1 = 0;
  byte param2 = 0;
  byte param3 = 0;
  byte program = 0;

  void saveTo(byte index);
  void loadFrom(byte index);
};
 
extern Preset currentPreset;
extern byte midiMap[32];
extern byte currentPresetNumber;
extern byte currentMidiMappingIndex;

extern byte presetEncoderValue;
extern byte param1EncoderValue;
extern byte param2EncoderValue;
extern byte param3EncoderValue;
extern byte receivedMidiProgrammIndex;

void saveMidiMap();
void restoreMidiMap();

#endif 