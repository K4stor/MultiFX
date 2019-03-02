#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <EepromAbstraction.h>
#include <EepromAbstractionWire.h>
#include "ApplicationModel.h"
#include "DisplayHelpers.h"

// PINS
const int presetButtonPin = 8;
const int param1ButtonPin = 7;

// Button states  HIGH means NOT pressed down.
int param1ButtonState = HIGH;
int presetButtonState = HIGH;

bool presetButtonLongPress = false;
long presetButtonTimer = 0;

const long longPressTime = 2000;

HardwareRotaryEncoder *presetEncoder;
HardwareRotaryEncoder *param1Encoder;
HardwareRotaryEncoder *param2Encoder;
HardwareRotaryEncoder *param3Encoder;

I2cAt24Eeprom anEeprom(0x50, 32);

// foreward declaration of change handlers
void onPresetEncoderChange(int newValue);
void onParam1EncoderChange(int newValue);
void onParam2EncoderChange(int newValue);
void onParam3EncoderChange(int newValue);

// --- State machine
enum State {
  start,
  selectPresetToOpen,
  selectPresetToSave,
  editParameter1,
  editParameter2,
  editParameter3,
  editMidiMapping,
  editProgram,
  openSelectedPreset,
  saveSelectedPreset,
  saveMidiMapping,
  restoreMidiMapping
};

enum Event {
  turnPreset,
  turnPresetWithParam1Pressed,
  turnParam1,
  turnParam2,
  turnParam3,
  pressPreset,
  pressParam1,
  longPressPreset,
  longPressPresetWithParam1Pressed,
  loaded,
  saved,
  timer
};

struct Transition {
  enum State srcState;
  enum Event event;
  enum State destState;
  void (*function)(void); //call back utility
};

// handler foreward declaration
void transitionToOpenPreset();
void updatePresetToOpen();
void updatePresetToSave();
void openSelected();
void saveSelected();
void transitionToStart();
void transitionToEditParam1();
void transitionToEditParam2();
void transitionToEditParam3();
void transitionToSavePreset();
void updateParam1();
void updateParam2();
void updateParam3();
void transitionToEditProgram();
void updateProgram();
void transitionToEditMidiMapping();
void saveEditedMidiMapping();
void resetEditedMidiMapping();
void updateMidiFromParameter();
void updateMidiToParameter();

struct Transition transitions[] = {
    // branching from start
    {start, turnPreset, selectPresetToOpen, transitionToOpenPreset},
    {start, turnParam1, editParameter1, transitionToEditParam1},
    {start, turnParam2, editParameter2, transitionToEditParam2},
    {start, turnParam3, editParameter3, transitionToEditParam3},
    {start, longPressPreset, selectPresetToSave, transitionToSavePreset},
    {start, longPressPresetWithParam1Pressed, editMidiMapping, transitionToEditMidiMapping},
    {start, turnPresetWithParam1Pressed, editProgram, transitionToEditProgram},

    // branching from selectPresetToOpen
    {selectPresetToOpen, turnPreset, selectPresetToOpen, updatePresetToOpen},
    {selectPresetToOpen, pressPreset, openSelectedPreset, openSelected},
    {selectPresetToOpen, pressParam1, start, transitionToStart},

    // branching from selectPresetToSave
    {selectPresetToSave, turnPreset, selectPresetToSave, updatePresetToSave},
    {selectPresetToSave, pressPreset, saveSelectedPreset, saveSelected},
    {selectPresetToSave, pressParam1, start, transitionToStart},

    // branching from editParameter1
    {editParameter1, turnParam1, editParameter1, updateParam1},
    {editParameter1, pressParam1, start, transitionToStart},

    // branching from editParameter2
    {editParameter2, turnParam2, editParameter2, updateParam1},
    {editParameter2, pressParam1, start, transitionToStart},

    // branching from editParameter3
    {editParameter3, turnParam3, editParameter3, updateParam1},
    {editParameter3, pressParam1, start, transitionToStart},

    // branching from openSelectedPreset
    {openSelectedPreset, loaded, start, transitionToStart},

    // branching from saveSelectedPreset
    {saveSelectedPreset, saved, start, transitionToStart},

    // branching from editProgram
    {editProgram, turnPresetWithParam1Pressed, editProgram, updateProgram},
    {editProgram, pressParam1, start, transitionToStart},

    // bracnching from editMidiMapping
    {editMidiMapping, turnParam1, editMidiMapping, updateMidiFromParameter},
    {editMidiMapping, turnPreset, editMidiMapping, updateMidiToParameter},
    {editMidiMapping, pressParam1, restoreMidiMapping, resetEditedMidiMapping},
    {editMidiMapping, longPressPreset, saveMidiMapping, saveEditedMidiMapping},

    {restoreMidiMapping, loaded, start, transitionToStart},
    {saveMidiMapping, saved, start, transitionToStart}
};

State currentState = start;
bool muteEvents = false;
bool presetTurnedWhileParam1Down = false;

void handleEvent(Event event) {
  if (muteEvents) {
    return;
  }

  int count = sizeof(transitions) / sizeof(transitions[0]);
  for (int i = 0; i < count; i++) {
    if ((transitions[i].srcState == currentState) && (transitions[i].event == event)) {
      currentState = transitions[i].destState;
      transitions[i].function();
    }
  }
}

// ------------------- Buttons -> Event
void updateParam1ButtonState() {
  int buttonState = digitalRead(param1ButtonPin);
  if (buttonState != param1ButtonState) {
    param1ButtonState = buttonState;
    // was LOW before therefore user just lifted it up
    if (param1ButtonState == HIGH) {
      if (!presetTurnedWhileParam1Down) {
        handleEvent(pressParam1);
      }
    }
    else {
      // was LOW before therefore user just pressed it down
      Serial.println("Gettin down");
      presetTurnedWhileParam1Down = false;
    }
  }
}

void updatePresetButtonState() {
  int buttonState = digitalRead(presetButtonPin);
  if (buttonState != presetButtonState) {
    presetButtonState = buttonState;
    if (presetButtonState == HIGH) {
      if (!presetButtonLongPress) {
        handleEvent(pressPreset);
      }
      presetButtonLongPress = false;
    } else {
      presetButtonTimer = millis();
    }
  }

  if (buttonState == LOW && !presetButtonLongPress) {
    if (millis() - presetButtonTimer > longPressTime) {
      presetButtonLongPress = true;
      if (param1ButtonState == HIGH) {
        handleEvent(longPressPreset);
      } else {
        handleEvent(longPressPresetWithParam1Pressed);
      }
    }
  }
}

void updateButtonStates() {
  updateParam1ButtonState();
  updatePresetButtonState();
}

// ------------------- Encoders -> Event
void onPresetEncoderChange(int newValue) {
  presetEncoderValue = newValue > 0 ? newValue : 1;
  Serial.println(param1ButtonState);
  if (param1ButtonState == HIGH) {
    handleEvent(turnPreset);
  }
  else {
    presetTurnedWhileParam1Down = true;
    handleEvent(turnPresetWithParam1Pressed);
  }
}

void onParam1EncoderChange(int newValue) {
  param1EncoderValue = newValue;
  handleEvent(turnParam1);
}

void onParam2EncoderChange(int newValue) {
  param2EncoderValue = newValue;
  handleEvent(turnParam2);
}

void onParam3EncoderChange(int newValue) {
  param3EncoderValue = newValue;
  handleEvent(turnParam3);
}

// ------------------ Setup
void setupEncoders() {
  switches.initialiseInterrupt(ioFrom8754(0x20, 2), true);
  presetEncoder = new HardwareRotaryEncoder(0, 1, onPresetEncoderChange);
  param1Encoder = new HardwareRotaryEncoder(2, 3, onParam1EncoderChange);
  param2Encoder = new HardwareRotaryEncoder(4, 5, onParam2EncoderChange);
  param3Encoder = new HardwareRotaryEncoder(6, 7, onParam3EncoderChange);

  switches.setEncoder(0, presetEncoder);
  switches.setEncoder(1, param1Encoder);

  muteEvents = true;
  switches.changeEncoderPrecision(0, 32, 1);
  switches.changeEncoderPrecision(1, 127, 0);
  muteEvents = false;
}

void setupButtons() {
  pinMode(presetButtonPin, INPUT);
}

void setup() {
  Serial.begin(9600); // open the serial port at 9600 bps:
  Wire.begin();

  setupDisplay();
  setupButtons();
  setupEncoders();
  drawNumber(currentPresetNumber);
  transitionToStart();
  // anEeprom.write8(1, 123);
  // byte by = anEeprom.read8(1);
  // Serial.println(by);
}

void loop() {
  updateButtonStates();
  taskManager.runLoop();
}

// -------------------- Event handler
void transitionToOpenPreset() {
  dotIndex = DI_NONE;
  startBlink();
  muteEvents = true;
  presetEncoder->changePrecision(32, currentPresetNumber);
  muteEvents = false;
  drawNumber(currentPresetNumber);
}

void updatePresetToOpen() {
  drawNumber(presetEncoderValue);
}

void openSelected() {
  currentPresetNumber = presetEncoderValue;
  currentPreset.loadFrom(currentPresetNumber);
  stopBlink();
  showDone();
  delay(200);
  handleEvent(loaded);
}

void transitionToStart() {
  dotIndex = DI_NONE;
  muteEvents = true;
  presetEncoder->changePrecision(32, currentPresetNumber);
  muteEvents = false;
  drawNumber(currentPresetNumber);
}

void transitionToEditParam1() {
  dotIndex = DI_FIRST;
  stopBlink();
  muteEvents = true;
  presetEncoder->changePrecision(127, currentPreset.param1);
  muteEvents = false;
  drawNumber(currentPreset.param1);
}

void transitionToEditParam2() {
  dotIndex = DI_SECOND;
  stopBlink();
  muteEvents = true;
  presetEncoder->changePrecision(127, currentPreset.param2);
  muteEvents = false;
  drawNumber(currentPreset.param2);
}

void transitionToEditParam3() {
  dotIndex = DI_THIRD;
  stopBlink();
  muteEvents = true;
  presetEncoder->changePrecision(127, currentPreset.param3);
  muteEvents = false;
  drawNumber(currentPreset.param3);
}

void updateParam1() {
  currentPreset.param1 = param1EncoderValue;
  drawNumber(currentPreset.param1);
}

void updateParam2() {
  currentPreset.param2 = param2EncoderValue;
  drawNumber(currentPreset.param2);
}

void updateParam3() {
  currentPreset.param3 = param3EncoderValue;
  drawNumber(currentPreset.param3);
}

void transitionToSavePreset() {
  dotIndex = DI_NONE;
  startBlink();
  muteEvents = true;
  presetEncoder->changePrecision(32, currentPresetNumber);
  muteEvents = false;

  drawNumber(currentPresetNumber);
}

void updatePresetToSave() {
  drawNumber(presetEncoderValue);
}

void saveSelected() {
  currentPresetNumber = presetEncoderValue;
  currentPreset.saveTo(currentPresetNumber);
  stopBlink();
  showDone();
  delay(200);
  handleEvent(saved);
}

void transitionToEditProgram() {
  dotIndex = DI_FOURTH;
  stopBlink();
  muteEvents = true;
  presetEncoder->changePrecision(8, currentPreset.program);
  muteEvents = false;
  drawNumber(currentPreset.program);
}

void updateProgram() {
  currentPreset.program = presetEncoderValue;
  drawNumber(currentPreset.program);
}

void transitionToEditMidiMapping() {
  dotIndex = DI_NONE;
  muteEvents = true;
  param1Encoder->changePrecision(32, currentMidiMappingIndex);
  presetEncoder->changePrecision(32, midiMap[currentMidiMappingIndex]);
  muteEvents = false;
}

void saveEditedMidiMapping() {
  saveMidiMap();
}

void resetEditedMidiMapping() {
  restoreMidiMap();
}

void updateMidiFromParameter() {
  currentMidiMappingIndex = param1EncoderValue;
  drawTwoBytes(currentMidiMappingIndex, midiMap[currentMidiMappingIndex]);
}

void updateMidiToParameter() {
  midiMap[currentMidiMappingIndex] = presetEncoderValue; 
  drawTwoBytes(currentMidiMappingIndex, midiMap[currentMidiMappingIndex]);
}
