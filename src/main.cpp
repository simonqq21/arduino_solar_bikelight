#include <Arduino.h>
#include "buttonlib2.h"
#include "LED.h"
#include "LowPower.h"
#include "math.h"

/**
 * low LEDs - 7 dim red LEDs 
 * high LEDs - 4 bright red LEDs 
 * charging LED - 1 dim red LED (identical to low LEDs, also used for rear light)
 */

int btn1Pin = 2;
InterruptButton btn1(btn1Pin);
int lowLEDPin = 4;
int chargingLEDsPin = 5;
int highLEDPin = 6;
int chargingPin = A0;
LED led1(lowLEDPin);
LED led2(chargingLEDsPin);
LED led3(highLEDPin);

/**
 * Modes: 
 * 0 - off - all LEDs off
 * 1 - low - low LEDs on, charging LEDs on, high LEDs off
 * 2 - high - low LEDs on, charging LEDs on, high LEDs on
 * 3 - flashing - low LEDs on, charging LEDs on, high LEDs flashing
 * 4 - fading - low LEDs on, charging LEDs on, high LEDs fading
 * 5 - charging(either via USB port or solar panel) - low LEDs off, charging LEDs on, high LEDs off
 */
int curMode = 0;

bool debug = true;

void btn1_change_func() {
  btn1.changeInterruptFunc();
}

void offMode() {
  
}

void lowMode() {

}

void highMode() {

}

void flashingMode() {

}

void fadingMode() {

}

void chargingMode() {

}

void setup() {
  if (debug) Serial.begin(115200);
  btn1.begin(btn1_change_func);
  btn1.set1ShortPressFunc();
  pinMode(lowLEDPin, OUTPUT);
  pinMode(chargingLEDsPin, OUTPUT);
  pinMode(highLEDPin, OUTPUT);
  // LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, )
  
}

void loop() {
  btn1.loop();

}

