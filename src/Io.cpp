#include "Io.h"
#include <EepromAbstraction.h>
#include <EepromAbstractionWire.h>


#define EMULATE_EEPROM // for development I use ram. That way the eeprom wont ware off.

#ifdef EMULATE_EEPROM
byte memory[200];
#endif

I2cAt24Eeprom eeprom(0x50, 32);

bool isMemoryInitialized() {
  #ifdef EMULATE_EEPROM
  return memory[0] == 'M' && memory[1] == 'F' && memory[2] == 'X'; 
  #endif
}

void factoryReset() {
  #ifdef EMULATE_EEPROM
  // write signature
  memory[0] = 'M';
  memory[1] = 'F';  
  memory[2] = 'X';
    
  Preset emptyPreset;
  
  // write presets
  for (int i = 0; i < PRESET_COUNT; i++) {
    writePresetData(emptyPreset, i);
  }

  // write midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    memory[i] = midiMap[i];
  }

  #endif
}

void writePresetData(Preset preset, byte index) {
  int offset = SIGNATURE_LENGTH + index * PRESET_LENGTH;

  #ifdef EMULATE_EEPROM
  memory[offset] = preset.program;
  memory[offset + 1] = preset.param1;
  memory[offset + 2] = preset.param2;
  memory[offset + 3] = preset.param3;
  #endif
}

void writeMidiMapping() {
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  // write midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    memory[offset] = midiMap[i];
    offset++;
  }
  #endif
}

void readPresetData(byte index) {
  int offset = SIGNATURE_LENGTH + index * PRESET_LENGTH;

  #ifdef EMULATE_EEPROM
  currentPreset.program = memory[offset];
  currentPreset.param1 = memory[offset + 1];
  currentPreset.param2 = memory[offset + 2];
  currentPreset.param3 = memory[offset + 3];
  #endif
}

void readMidiMap() {
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  // write midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    midiMap[i] = memory[offset];
    offset++;
  }
  #endif
}

void writeLastUsedPresetIndex(byte index) {
  // offset = signature + presets + midi map
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT + PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  memory[offset] = index;
  #endif
}

byte readLastUsedPresetIndex() {
  // offset = signature + presets + midi map
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT + PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  return memory[offset];
  #endif
}

void setupProgramPins() {
  pinMode(S0_PIN, OUTPUT);
  pinMode(S1_PIN, OUTPUT);
  pinMode(S2_PIN, OUTPUT);
}

void writeProgramPins(byte program) {
  digitalWrite(S0_PIN, (program & 1));
  digitalWrite(S1_PIN, (program & 2));
  digitalWrite(S2_PIN, (program & 4));
}