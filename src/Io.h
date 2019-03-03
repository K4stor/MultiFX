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

bool isMemoryInitialized();
void factoryReset();
void writePresetData(Preset preset, byte index);
void writeMidiMapping();

void readPresetData(byte index);
void readMidiMap();

void setupProgramPins();
void writeProgramPins(byte program);

#endif