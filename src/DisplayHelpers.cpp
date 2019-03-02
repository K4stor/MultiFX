#include <DisplayHelpers.h>

#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>

// LETTERS 
#define LED_S 0b01101101
#define LED_A 0b01110111
#define LED_E 0b01111001
#define LED_D 0b01011110
#define LED_N 0b00110111
#define LED_O 0b00111111
#define LED_V 0b00111110

#define FIRST_DIGIT_INDEX 0
#define SECOND_DIGIT_INDEX 1
#define THIRD_DIGIT_INDEX 3
#define FOURTH_DIGIT_INDEX 4

Adafruit_7segment matrix = Adafruit_7segment();

int dotIndex = 0;

// ---- LED Helpers 
void drawByteOnTwoDigits(byte value, byte startIndex) {
  if (value > 9)  {
    matrix.writeDigitNum(startIndex, value / 10);
  } else {
    matrix.writeDigitRaw(startIndex, 0);    
  }
   matrix.writeDigitNum(startIndex + 1, value % 10);  
}

void drawTwoBytes(byte value1, byte value2) {
   drawByteOnTwoDigits(value1, FIRST_DIGIT_INDEX);
   drawByteOnTwoDigits(value2, THIRD_DIGIT_INDEX);
   matrix.drawColon(true);
   matrix.writeDisplay();
}

int drawDigit(int value, int base, byte index, bool clearDigit) {
  int result = value;
  bool drawDot = index == dotIndex;
  if (value >= base)  {
    int quotient = value / base;
    matrix.writeDigitNum(index, quotient, drawDot);
    result -= quotient * base;
  } else {
    if (clearDigit) {
      matrix.writeDigitRaw(index, drawDot << 7);    
    }
    else {
      matrix.writeDigitNum(index, 0, drawDot);      
    }
  }  
  return result;
}

void drawNumber(int value) {
  int newValue = value;
  newValue = drawDigit(newValue, 1000, FIRST_DIGIT_INDEX, value < 1000);   
  newValue = drawDigit(newValue, 100, SECOND_DIGIT_INDEX, value < 100);
  newValue = drawDigit(newValue, 10, THIRD_DIGIT_INDEX, value < 10);
  
  matrix.writeDigitNum(FOURTH_DIGIT_INDEX, newValue % 10);  
  matrix.writeDisplay();
}

void startBlink() {
  matrix.blinkRate(HT16K33_BLINK_2HZ);
}

void stopBlink() {
  matrix.blinkRate(HT16K33_BLINK_OFF);
}

void setupDisplay() {
  matrix.begin(0x70);
}

void showDone() {
  matrix.writeDigitRaw(FIRST_DIGIT_INDEX, LED_D);    
  matrix.writeDigitRaw(SECOND_DIGIT_INDEX, LED_O);    
  matrix.writeDigitRaw(THIRD_DIGIT_INDEX, LED_N);    
  matrix.writeDigitRaw(FOURTH_DIGIT_INDEX, LED_E);  
  matrix.writeDisplay();  
}

void hideColon() {
  matrix.drawColon(false);
  matrix.writeDisplay();
}