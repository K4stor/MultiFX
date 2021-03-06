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
  #else
  return eeprom.read8(0) == 'M' && eeprom.read8(1) == 'F' && eeprom.read8(2) == 'X';
  #endif
}

void factoryReset() {
  #ifdef EMULATE_EEPROM
  // write signature
  memory[0] = 'M';
  memory[1] = 'F';  
  memory[2] = 'X';
  #else
  eeprom.write8(0, 'M');
  eeprom.write8(1, 'F');
  eeprom.write8(2, 'X');
  #endif
    
  Preset emptyPreset;
  
  for (int i = 0; i < PRESET_COUNT; i++) {
    writePresetData(emptyPreset, i);
  }

  // reset midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    midiMap[i] = i;
  }

  writeMidiMapping();
}

void writePresetData(Preset preset, byte index) {
  int offset = SIGNATURE_LENGTH + index * PRESET_LENGTH;

  #ifdef EMULATE_EEPROM
  memory[offset] = preset.program;
  memory[offset + 1] = preset.param1;
  memory[offset + 2] = preset.param2;
  memory[offset + 3] = preset.param3;
  #else 
  eeprom.write8(offset, preset.program);
  eeprom.write8(offset + 1, preset.param1);
  eeprom.write8(offset + 2, preset.param2);
  eeprom.write8(offset + 3, preset.param3);
  #endif
}

void writeMidiMapping() {
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT;
  // write midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    #ifdef EMULATE_EEPROM
    memory[offset] = midiMap[i];
    #else
    eeprom.write8(offset, midiMap[i]);
    #endif
    offset++;
  }
}

void readPresetData(byte index) {
  int offset = SIGNATURE_LENGTH + index * PRESET_LENGTH;

  #ifdef EMULATE_EEPROM
  currentPreset.program = memory[offset];
  currentPreset.param1 = memory[offset + 1];
  currentPreset.param2 = memory[offset + 2];
  currentPreset.param3 = memory[offset + 3];
  #else
  currentPreset.program = eeprom.read8(offset);
  currentPreset.param1 = eeprom.read8(offset + 1);
  currentPreset.param2 = eeprom.read8(offset + 2);
  currentPreset.param3 = eeprom.read8(offset + 3);
  #endif
}

void readMidiMap() {
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT;
  // read midi map
  for (int i = 0; i < PRESET_COUNT; i++) {
    #ifdef EMULATE_EEPROM
    midiMap[i] = memory[offset];
    #else
    midiMap[i] = eeprom.read8(offset);
    #endif
    offset++;
  }
}

void writeLastUsedPresetIndex(byte index) {
  // offset = signature + presets + midi map
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT + PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  memory[offset] = index;
  #else 
  eeprom.write8(offset, index);
  #endif
}

byte readLastUsedPresetIndex() {
  // offset = signature + presets + midi map
  int offset = SIGNATURE_LENGTH + PRESET_LENGTH * PRESET_COUNT + PRESET_COUNT;
  #ifdef EMULATE_EEPROM
  return memory[offset];
  #else
  return eeprom.read8(offset);
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

void setupPWNPins() {
  pinMode(POT0_PIN, OUTPUT);
  pinMode(POT1_PIN, OUTPUT);
  pinMode(POT2_PIN, OUTPUT);
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM10);
  TCCR1B = _BV(WGM12) | _BV(CS10);
  TCCR2A = _BV(WGM20) | _BV(COM2A1) | _BV(WGM21) | _BV(CS20);
  
  // Reset parameter pins
  OCR1A = 0;
  OCR1B = 0;
  OCR2A = 0;
}

byte remapValue(byte value) {
  // scale to 0..1
  double normalizedValue = (double)value / 255.0;
  double scaledValue = floor(normalizedValue * 255.0);
  return (byte)scaledValue;
}

void writeParam1Pin(byte value) {
  byte mappedValue = remapValue(value);
  Serial.println(mappedValue);
  OCR1A = mappedValue;
}

void writeParam2Pin(byte value) {
  byte mappedValue = remapValue(value);
  OCR1B = mappedValue;
}

void writeParam3Pin(byte value) {
  byte mappedValue = remapValue(value);
  OCR2A = mappedValue;
}
