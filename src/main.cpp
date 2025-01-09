#include <Arduino.h>
#include "LowPower.h"
#include "math.h"
#include "EEPROM.h"
#include "buttonlib2.h"
#include "LED.h"

/**
 * low LEDs - 7 dim red LEDs 
 * high LEDs - 4 bright red LEDs 
 * charging LED - 1 dim red LED (identical to low LEDs, also used for rear light)
 */

int btn1Pin = 2;
InterruptButton btn1(btn1Pin);
int lowLEDsPin = 4;
int chargingLEDsPin = 5;
int highLEDsPin = 6;
int chargingPin = A0;
LED lowLEDs(lowLEDsPin);
LED chargingLEDs(chargingLEDsPin);
LED highLEDs(highLEDsPin);

/**
 * Modes: 
 * 0 - off - all LEDs off
 * 1 - low - low LEDs on, charging LEDs on, high LEDs off
 * 2 - high - low LEDs on, charging LEDs on, high LEDs on
 * 3 - flashing - low LEDs on, charging LEDs on, high LEDs flashing
 * 4 - fading - low LEDs on, charging LEDs on, high LEDs fading
 * 5 - charging(either via USB port or solar panel) - low LEDs off, charging LEDs on, high LEDs off
 * 
 * charging LED is off when (not charging and off), and on when (the low LEDs are on or when charging)
 */
int curMode = 0;
int chargingPinADCVal;

// update period for fading modes
unsigned int updatePeriodinMillis = 5;
// period length for flashing and fading modes
int totalPeriodLengthinMillis = 1000;
// keypoints per mode cycle 
unsigned int keyPoints[4];
// flashCycleTimer - used to keep time
unsigned long flashCycleTimer;
/**
 * ctr1 is a counter variable used to animate the LEDs.
 * ctr1 is measured in multiples of updatePeriodinMillis milliseconds, which varies 
 * depending on the mode. For example, it could be in multiples of 5 ms for fading, or 100 ms
 * for flashing.
 */
unsigned int ctr1;

bool debug = true;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

/**
 * single click - switch and cycle through modes
 */
void btn1_1shortclick_func() {
  // activateAutoSave();
  curMode++;
  // Serial.println(configuration->curBrightness > PWR_HIGH);
  if (curMode > 4) curMode = 0;
  Serial.print("curMode = ");
  Serial.println(curMode);
}

void offMode() {
  lowLEDs.off();
  highLEDs.off();
}

void lowMode() {
  lowLEDs.on();
  highLEDs.off();
}

/**
 * 
 */
void highMode() {
  lowLEDs.on();
  highLEDs.on();
}

/**
 * 
 */
void flashingMode() {
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300/updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis/updatePeriodinMillis;
  lowLEDs.on();
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      highLEDs.set(true);
    } else highLEDs.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1 + 1;
  }
}

/**
 * 
 */
void fadingMode() {
  updatePeriodinMillis = 5;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 400/updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 400/updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 200/updatePeriodinMillis;
  // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,80,160,200
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      highLEDs.aSet(sin(0.5 * PI * (ctr1-keyPoints[0]) / (keyPoints[1] - keyPoints[0]))*255);
      // curBrightnessVal = sin8((ctr1-keyPoints[0])*64/(keyPoints[1] - keyPoints[0])) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      highLEDs.aSet(sin(0.5 * PI * (1 + (ctr1-keyPoints[1]) / (keyPoints[2] - keyPoints[1])))*255);
      // curBrightnessVal = sin8((ctr1-keyPoints[1])*64/(keyPoints[2] - keyPoints[1])+64) * BRIGHTNESS_VALUES[configuration->curBrightness] / 255;
    } 
    else if (ctr1 >= keyPoints[2]) {
      highLEDs.aSet(0);
    }
    ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1 + 1;
  }
}

void updateChargingLEDs() {
  if (true || curMode != 0) {
    chargingLEDs.on();
  } else chargingLEDs.off();
}

void setup() {
  if (debug) Serial.begin(115200);
  btn1.begin(btn1_change_func);
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  lowLEDs.begin();
  chargingLEDs.begin();
  highLEDs.begin();
  // LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, )
}

void loop() {
  btn1.loop();
  lowLEDs.loop();
  chargingLEDs.loop();
  highLEDs.loop();

  switch (curMode) {
    case 1:

      break;
    default: 
      break;
  }
}

