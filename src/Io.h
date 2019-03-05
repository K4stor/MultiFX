#ifndef IO_H
#define IO_H

#include <Arduino.h>
#include "ApplicationModel.h"

#define SIGNATURE_LENGTH 3
#define PRESET_LENGTH 4
#define PRESET_COUNT 32

#define S0_PIN 4
#define S1_PIN 5
#define S2_PIN 6
#define PARAM1_BUTTON_PIN 7
#define PRESET_BUTTON_PIN 8
#define POT0_PIN 9
#define POT1_PIN 10
#define POT2_PIN 11

bool isMemoryInitialized();
void factoryReset();
void writePresetData(Preset preset, byte index);
void writeMidiMapping();

void readPresetData(byte index);
void readMidiMap();

void setupPWNPins();
void writeParam1Pin(byte value);
void writeParam2Pin(byte value);
void writeParam3Pin(byte value);

void setupProgramPins();
void writeProgramPins(byte program);

void writeLastUsedPresetIndex(byte index);
byte readLastUsedPresetIndex();

#endif