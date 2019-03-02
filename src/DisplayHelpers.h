#ifndef DISPLAY_HELPERS_H
#define DISPLAY_HELPERS_H

#include <Arduino.h>

enum DotIndex {
  DI_NONE = -1,
  DI_FIRST,
  DI_SECOND,
  DI_THIRD,
  DI_FOURTH
};

extern int dotIndex;

void drawTwoBytes(byte value1, byte value2);

void drawNumber(int value);

void setupDisplay();

void startBlink();

void stopBlink();

void showDone();

#endif