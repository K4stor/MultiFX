#ifndef DISPLAY_HELPERS_H
#define DISPLAY_HELPERS_H

#include <Arduino.h>

// We don't count 2 cos 2 is the column
enum DotIndex {
  DI_NONE = -1,
  DI_FIRST = 0,
  DI_SECOND = 1,
  DI_THIRD = 3,
  DI_FOURTH = 4
};

extern int dotIndex;

void drawTwoBytes(byte value1, byte value2);

void drawNumber(int value);

void setupDisplay();

void startBlink();

void stopBlink();

void showDone();

void hideColon();

#endif