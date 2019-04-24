#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
#include <IoAbstraction.h>
#include <IoAbstractionWire.h>
#include <MIDI.h>
#include "ApplicationModel.h"
#include "DisplayHelpers.h"
#include "Io.h"

#define MAX_PRESET_ENCODER_VALUE 31
#define MAX_PARAMETER_ENCODER_VALUE 255
#define MAX_PROGRAM_ENCODER_VALUE 7
#define DONE_DISPLAY_TIME 300

MIDI_CREATE_DEFAULT_INSTANCE();

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
  restoreMidiMapping,
  processMidiData
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
  operationFinished,
  timer,
  midiProgramCommand
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
void openPresetFromMidi();

struct Transition transitions[] = {
    // branching from start
    {start, turnPreset, selectPresetToOpen, transitionToOpenPreset},
    {start, turnParam1, editParameter1, transitionToEditParam1},
    {start, turnParam2, editParameter2, transitionToEditParam2},
    {start, turnParam3, editParameter3, transitionToEditParam3},
    {start, longPressPreset, selectPresetToSave, transitionToSavePreset},
    {start, longPressPresetWithParam1Pressed, editMidiMapping, transitionToEditMidiMapping},
    {start, turnPresetWithParam1Pressed, editProgram, transitionToEditProgram},
    {start, midiProgramCommand, processMidiData,  openPresetFromMidi},

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
    {editParameter2, turnParam2, editParameter2, updateParam2},
    {editParameter2, pressParam1, start, transitionToStart},

    // branching from editParameter3
    {editParameter3, turnParam3, editParameter3, updateParam3},
    {editParameter3, pressParam1, start, transitionToStart},

    // branching from openSelectedPreset
    {openSelectedPreset, operationFinished, start, transitionToStart},

    // branching from saveSelectedPreset
    {saveSelectedPreset, operationFinished, start, transitionToStart},

    // branching from editProgram
    {editProgram, turnPresetWithParam1Pressed, editProgram, updateProgram},
    {editProgram, pressParam1, start, transitionToStart},

    // bracnching from editMidiMapping
    {editMidiMapping, turnParam1, editMidiMapping, updateMidiFromParameter},
    {editMidiMapping, turnPreset, editMidiMapping, updateMidiToParameter},
    {editMidiMapping, pressParam1, restoreMidiMapping, resetEditedMidiMapping},
    {editMidiMapping, longPressPreset, saveMidiMapping, saveEditedMidiMapping},

    {restoreMidiMapping, operationFinished, start, transitionToStart},
    {saveMidiMapping, operationFinished, start, transitionToStart},

    {processMidiData , operationFinished, openSelectedPreset, openSelected}
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
  int buttonState = digitalRead(PARAM1_BUTTON_PIN);
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
      presetTurnedWhileParam1Down = false;
    }
  }
}

void updatePresetButtonState() {
  int buttonState = digitalRead(PRESET_BUTTON_PIN);
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

void handleProgramChange(byte channel, byte number) {
  receivedMidiProgrammIndex = number;
  handleEvent(midiProgramCommand);
}

// ------------------- Encoders -> Event
void onPresetEncoderChange(int newValue) {
  presetEncoderValue = newValue;
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
  switches.setEncoder(2, param2Encoder);
  switches.setEncoder(3, param3Encoder);

  muteEvents = true;
  byte lastUsedPresetIndex = readLastUsedPresetIndex();
  switches.changeEncoderPrecision(0, MAX_PRESET_ENCODER_VALUE, lastUsedPresetIndex);
  switches.changeEncoderPrecision(1, MAX_PARAMETER_ENCODER_VALUE, 0);
  switches.changeEncoderPrecision(2, MAX_PARAMETER_ENCODER_VALUE, 0);
  switches.changeEncoderPrecision(3, MAX_PARAMETER_ENCODER_VALUE, 0);
  muteEvents = false;
}

void setupButtons() {
  pinMode(PRESET_BUTTON_PIN, INPUT);
}

void setupSetupMemory() {
   if (!isMemoryInitialized()) {
    Serial.println("Nust clean");
    factoryReset();
  } else {
    Serial.println("no clean today");
  }
  byte lastUsedPresetIndex = readLastUsedPresetIndex();
  currentPreset.loadFrom(lastUsedPresetIndex);
}

void createInitialPinState() {
  writeProgramPins(currentPreset.program);
  writeParam1Pin(currentPreset.param1);
  writeParam2Pin(currentPreset.param2);
  writeParam3Pin(currentPreset.param3);
}

void setupMidi() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.setHandleProgramChange(handleProgramChange);
}

void setup() {
  Serial.begin(9600); // open the serial port at 9600 bps:
  Wire.begin();
  setupProgramPins();
  setupPWNPins();
  setupSetupMemory();
  setupDisplay();
  setupButtons();
  setupEncoders();
  setupMidi();
  createInitialPinState();
  transitionToStart();
}

void loop() {
  updateButtonStates();
  taskManager.runLoop();
  MIDI.read();
}

// -------------------- Event handler
void transitionToOpenPreset() {
  dotIndex = DI_NONE;
  startBlink();
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, currentPresetNumber);
  muteEvents = false;
  drawNumber(currentPresetNumber + 1);
}

void updatePresetToOpen() {
  drawNumber(presetEncoderValue + 1);
}

void openSelected() {
  currentPresetNumber = presetEncoderValue;
  currentPreset.loadFrom(currentPresetNumber);
  createInitialPinState();
  writeLastUsedPresetIndex(currentPresetNumber);
  stopBlink();
  handleEvent(operationFinished);
}

void transitionToStart() {
  dotIndex = DI_NONE;
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, currentPresetNumber);
  muteEvents = false;
  drawNumber(currentPresetNumber + 1);
}

void transitionToEditParam1() {
  dotIndex = DI_FIRST;
  stopBlink();
  muteEvents = true;
  param1Encoder->changePrecision(MAX_PARAMETER_ENCODER_VALUE, currentPreset.param1);
  muteEvents = false;
  drawNumber(currentPreset.param1);
}

void transitionToEditParam2() {
  dotIndex = DI_SECOND;
  stopBlink();
  muteEvents = true;
  param2Encoder->changePrecision(MAX_PARAMETER_ENCODER_VALUE, currentPreset.param2);
  muteEvents = false;
  drawNumber(currentPreset.param2);
}

void transitionToEditParam3() {
  dotIndex = DI_THIRD;
  stopBlink();
  muteEvents = true;
  param3Encoder->changePrecision(MAX_PARAMETER_ENCODER_VALUE, currentPreset.param3);
  muteEvents = false;
  drawNumber(currentPreset.param3);
}

void updateParam1() {
  currentPreset.param1 = param1EncoderValue;
  writeParam1Pin(currentPreset.param1);
  drawNumber(currentPreset.param1);
}

void updateParam2() {
  currentPreset.param2 = param2EncoderValue;
  writeParam2Pin(currentPreset.param2);
  drawNumber(currentPreset.param2);
}

void updateParam3() {
  currentPreset.param3 = param3EncoderValue;
  writeParam3Pin(currentPreset.param3);
  drawNumber(currentPreset.param3);
}

void transitionToSavePreset() {
  dotIndex = DI_NONE;
  startBlink();
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, currentPresetNumber);
  muteEvents = false;

  drawNumber(currentPresetNumber + 1);
}

void updatePresetToSave() {
  drawNumber(presetEncoderValue + 1);
}

void saveSelected() {
  currentPresetNumber = presetEncoderValue;
  currentPreset.saveTo(currentPresetNumber);
  stopBlink();
  showDone();
  delay(DONE_DISPLAY_TIME);
  handleEvent(operationFinished);
}

void transitionToEditProgram() {
  dotIndex = DI_FOURTH;
  stopBlink();
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PROGRAM_ENCODER_VALUE, currentPreset.program);
  muteEvents = false;
  drawNumber(currentPreset.program + 1);
}

void updateProgram() {
  currentPreset.program = presetEncoderValue;
  writeProgramPins(currentPreset.program);
  drawNumber(currentPreset.program + 1);
}

void transitionToEditMidiMapping() {
  currentMidiMappingIndex = 1;
  dotIndex = DI_NONE;
  muteEvents = true;
  param1Encoder->changePrecision(MAX_PRESET_ENCODER_VALUE, currentMidiMappingIndex);
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, midiMap[currentMidiMappingIndex]);
  muteEvents = false;
  drawTwoBytes(currentMidiMappingIndex + 1, midiMap[currentMidiMappingIndex] + 1);
}

void saveEditedMidiMapping() {
  hideColon();
  saveMidiMap();
  showDone();
  delay(DONE_DISPLAY_TIME);
  handleEvent(operationFinished);
}

void resetEditedMidiMapping() {
  hideColon();
  restoreMidiMap();
  showDone();
  delay(DONE_DISPLAY_TIME);
  handleEvent(operationFinished);
}

void updateMidiFromParameter() {
  currentMidiMappingIndex = param1EncoderValue;
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, midiMap[currentMidiMappingIndex]);
  muteEvents = false; 
  drawTwoBytes(currentMidiMappingIndex + 1, midiMap[currentMidiMappingIndex] + 1);
}

void updateMidiToParameter() {
  midiMap[currentMidiMappingIndex] = presetEncoderValue; 
  drawTwoBytes(currentMidiMappingIndex + 1, midiMap[currentMidiMappingIndex] + 1);
}

void openPresetFromMidi() {
  muteEvents = true;
  presetEncoder->changePrecision(MAX_PRESET_ENCODER_VALUE, midiMap[receivedMidiProgrammIndex]);
  muteEvents = false;
  handleEvent(operationFinished);
}
