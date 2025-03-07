/**
 * TODO: 
 * auto mode
 */
#include <Arduino.h>
#include "math.h"
#include "buttonlib2.h"
#include "LEDlib.h"
#include <avr/sleep.h>  
#include <avr/wdt.h> 
#include <EEPROM.h> 

/**
 * low LEDs - 7 dim red LEDs 
 * high LEDs - 4 bright red LEDs 
 * charging LED - 1 dim red LED (identical to low LEDs, also used for rear light)
 */

/**
  * 1 short press - cycle through modes 
  * 2 short press - cycle through mode memory, then at the end of each cycle, toggle the auto mode that uses the LDR to detect light. 
  * 1 long press - switch the light on or off 
  * 
  * Lighting modes: 
  * 0 - off - low LEDs 1 off, low LEDs 2 off, high LEDs off
  * 1 - extended low - low LEDs 1 on, low LEDs 2 flashing, high LEDs off
  * 2 - extended low 2 - low LEDs 1 on, low LEDs 2 fading, high LEDs off
  * 3 - low - low LEDs 1 on, low LEDs 2 on, high LEDs off
  * 4 - high - low LEDs 1 on, low LEDs 2 on, high LEDs on
  * 5 - flashing - low LEDs 1 on, low LEDs 2 on, high LEDs flashing
  * 6 - fading - low LEDs 1 on, low LEDs 2 on, high LEDs fading
  * 7 - dynamo flash - low LEDs 1 on, low LEDs 2 on, high LEDs rapid flashing
 * 
  * Mode memory modes: 
  * 1 - no mode memory - when connected to battery, light is off and long press starts the light in 
  *   the first mode in the sequence.
  * 2 - mode memory - when connected to battery, light is off and long press starts the light in 
  *   the last memorized mode.
  * 3 - mode memory and auto-on - when connected to battery, light is on at the last memorized mode.
  * 
  * Light sensing auto mode pseudocode 
  If Light sensing auto mode {
    If bike light is on {
      if LDR bright {
        switch bike light to extended low mode;
      } else if LDR dark {
        switch bike light to saved mode;
      }
    }
    else if bike light is off {
      switch bike light to off;
    }
  }
  else if not Light sensing auto mode {
    no change;
  }
 *
 *
 * 
 * 
*/
const int btn1Pin = 2;
InterruptButton btn1(btn1Pin);
const int lowLEDsPin = 4;
const int lowLEDs2Pin = 5;
const int highLEDsPin = 6;
const int ldrPin = A0;
const int batPin = A1;
LED lowLEDs(lowLEDsPin);
LED lowLEDs2(lowLEDs2Pin);
LED highLEDs(highLEDsPin);
unsigned int deadBatCountDown, lowBatCountDown; 

const int NUM_MODES = 7;
byte curLightMode = 0;
byte curMMMode; // current mode memory mode 
bool autoMode = false; 
int chargingPinReading;
double chargingPinVolts, batVolts;
bool lowBattery, deadBattery;
// bool isCharging; 
bool isSleeping; 

// update period for fading modes
unsigned int updatePeriodinMillis = 5;
// period length for flashing and fading modes
int totalPeriodLengthinMillis = 1000;
// keypoints per mode cycle 
unsigned int keyPoints[4];
// flashCycleTimer - used to keep time
unsigned long flashCycleTimer;
unsigned long powerOnTime; 
unsigned long lastTimeBtnDoubleClicked; 
unsigned long lastTimeBtnClicked;
// unsigned long lastTimeChargingVoltsExceeded;
// const double chargingThresholdVolts = 4.0;
const double deadBatVolts = 3.0;
const double lowBatVolts = 3.6;
const double hysteresisBatVolts = 0.1;

/**
 * ctr1 is a counter variable used to animate the LEDs.
 * ctr1 is measured in multiples of updatePeriodinMillis milliseconds, which varies 
 * depending on the mode. For example, it could be in multiples of 5 ms for fading, or 100 ms
 * for flashing.
 */
unsigned int ctr1;
unsigned long lastTimePrinted = 0;
const bool debug = true;

void startSleepTimer() {
  powerOnTime = millis();
  isSleeping = false;
}

/**
 * single click - switch and cycle through modes
 */
void btn1_1shortclick_func() {
  Serial.println("1 short click");
  // restart sleep timer
  startSleepTimer();
  lastTimeBtnClicked = millis();
  // if auto mode is turned on and the light is charging, 
  // set current mode to saved mode.
  // if (autoMode && isCharging) {
  //   curLightMode = savedMode;
  // }

  // // if the battery voltage is lower than the defined low battery 
  // // voltage plus a certain buffer value, it is considered battery low 
  // // when the button is pressed.
  // if (batVolts < lowBatVolts + hysteresisBatVolts) {
  //   lowBattery = true;
  // }


  // if the battery is low and the current mode is greater than extended low mode, 
  // set current mode to off mode.
  if ((lowBattery && curLightMode >= 1) || deadBattery) {
    curLightMode = 0;
  }
  // else, increment the current mode.
  else {
    curLightMode++;
  }

  // Cycle current mode to zero when it exceeds the last mode.
  if (curLightMode > NUM_MODES) {
    curLightMode = 0;
  }

  // if auto mode is turned on, 
  // set saved mode to current mode.
  // if (autoMode) {
  //   savedMode = curLightMode;
  // }
  if (debug) {
    Serial.print("curLightMode = ");
    Serial.println(curLightMode);
  }
  // reset watchdog timer
  wdt_reset();
}

/**
 * double click - toggle autoMode
 */
void btn1_2shortclick_func() {
  Serial.println("2 short click");
  lastTimeBtnDoubleClicked = millis();
  lastTimeBtnClicked = millis();
  startSleepTimer();
  autoMode = !autoMode; 
  lowLEDs2.setLoopUnitDuration(200);
  if (autoMode) {
    bool loopSeq[] = {0,1,0,1,0};
    lowLEDs2.startTimer(1000, true);
    lowLEDs2.setLoopSequence(loopSeq, 5);
    lowLEDs2.startLoop();
  } else {
    bool loopSeq[] = {0,1,0,0,0};
    lowLEDs2.startTimer(1000, true);
    lowLEDs2.setLoopSequence(loopSeq, 5);
    lowLEDs2.startLoop();
  }
  if (debug) {
    Serial.print("autoMode=");
    Serial.println(autoMode);
    Serial.print("mode=");
    Serial.print(curLightMode);
    Serial.println(); 
  }
  // reset watchdog timer
  wdt_reset();
  
}

void btn1_1longclick_func() {
  Serial.println("1 long click");
}

// ISR for interrupt button library 
void btn1_change_func() {
  btn1.changeInterruptFunc();
}

// ISR for waking up from deep sleep
void wakeupISR() {
  ADCSRA |= 1 << ADEN;
  startSleepTimer(); 
  btn1_1shortclick_func();
  btn1.begin(btn1_change_func);
  // reset button temporarily to prevent double trigger
  btn1.reset();
}



void checkBatVolts() {
  batVolts = analogRead(batPin)*1.1/1023.0*6;
  // turn off the light if the battery voltage is waaay too low
  if (batVolts <= deadBatVolts) {
    deadBatCountDown++;
    lowBatCountDown = 0;
    if (deadBatCountDown > 10) {
      deadBatCountDown = 0;
      curLightMode = 0;
      lowBattery = true;
      deadBattery = true;
    }
  } 
  // turn on the light in extended low mode if battery is low and the light is turned on 
  else if (batVolts > deadBatVolts && batVolts <= lowBatVolts) {
    deadBatCountDown = 0;
    lowBatCountDown++;
    if (lowBatCountDown > 10) {
      lowBatCountDown = 0;
      lowBattery = true;
      deadBattery = false;
    }
  } 
  else {
    deadBatCountDown = 0;
    lowBatCountDown = 0;
    lowBattery = false;
    deadBattery = false;
  }
  if (curLightMode > 1 && lowBattery) {
    curLightMode = 1;
  }
}

/**

If the light is in autoMode {
  If the light is charging {
    If the saved mode is different from the current mode {
      Save the current mode 
      // Set current mode to zero 
    }
    If the button is pressed {
      Set current mode to the saved mode
      Increment the current mode in a cycle 
      Start a timer lightsTimer that keeps the lights on five seconds 
        before it turns off due to autoMode and charging.
      When that timer exceeds 5 seconds {
        Save the current mode 
        Set current mode to zero
      }
    }
  }
  Else if the light is not charging {
    Resume the saved mode
  }
}
Else if the light is not in autoMode {
  Maintain the current mode 
  If the saved mode is nonzero and the current mode is zero {
    Resume the saved mode
  }
}

*/

// void checkAutoMode() {
//   if (!deadBattery) {
//     if (autoMode) {
//       // Serial.print("curLightMode = ");
//       // Serial.print(curLightMode);
//       // Serial.print(", savedmode = ");
//       // Serial.print(savedMode);
//       // Serial.print(" ischarging=");
//       // Serial.print(isCharging);
//       // Serial.println();
//       // if (isCharging) {
//         // Serial.print(chargingPinReading);
//         // Serial.print(", ");
//         // Serial.print(chargingPinVolts);
//         // Serial.print(", ");
//         // Serial.print(chargingThresholdVolts);
//         // Serial.print(", ");
//         // Serial.print("ischarging=");
//         // Serial.print(isCharging);
//         // Serial.println();
//         if (millis() - lastTimeBtnClicked > 4000) {
//           if (curLightMode) {
//             savedMode = curLightMode;
//             curLightMode = 0;
//           }
//         } 
//       } 
//       else {
//         if (curLightMode != savedMode && savedMode) {
//           curLightMode = savedMode;
//           btn1.begin(btn1_change_func);
//           // reset button temporarily to prevent double trigger
//           btn1.reset();
//         }
//       }
//     }
//   }
//   else {
//     // Serial.print("deadbattery=");
//     // Serial.print(deadBattery);
//     // Serial.print(", batvolts = ");
//     // Serial.print(batVolts);
//     // Serial.println();
//     savedMode = 0;
//   }
// }

// ISR triggered by watchdog timer every 1 seconds during deep sleep,
// before going back to sleep.
ISR (WDT_vect) {
  ADCSRA |= 1 << ADEN;
  wdt_reset();
  checkBatVolts(); 
  // updateChargeLED();
  // checkAutoMode();
  checkBatVolts();
}

/**
 * mode 0 - Off Mode
 */
void offMode() {
  lowLEDs.off();
  lowLEDs2.off();
  highLEDs.off();
  if (millis() - powerOnTime > 3000) {
    isSleeping = true;
    // set interrupt to perform the watchdog ISR every four seconds then go back to sleep
    // clear MCU Status Register
    MCUSR = 0;
    // set watchdog timer change enable and watchdog enable
    WDTCSR = 1 << WDCE | 1 << WDE;
    // set WDP3 to WDP0 to trigger watchdog interrupt every 4 seconds and 
    // set WDIE enable watchdog interrupt
    // WDTCSR = bit(WDIE) | 1 << WDP3 & ~bit (WDP2) & ~bit (WDP1) & ~bit (WDP0);
    // WDTCSR = 1 << WDIE | 1 << WDP3 | 0 << WDP2 | 0 << WDP1 | 0 << WDP0;
    WDTCSR = 1 << WDIE | 0 << WDP3 | 1 << WDP2 | 1 << WDP1 | 0 << WDP0;

    // sleep CPU until woken up by the button
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    detachInterrupt(digitalPinToInterrupt(2));
    attachInterrupt(digitalPinToInterrupt(2), wakeupISR, LOW);  
    // sleep_mode(); 
    // sei();
    ADCSRA = 0;
    // turn off brown-out enable in software
    MCUCR = bit (BODS) | bit (BODSE);  // turn on brown-out enable select
    MCUCR = bit (BODS);        // this must be done within 4 clock cycles of above
    interrupts ();             // guarantees next instruction executed

    sleep_cpu();
    // sleep_disable();
    // sei();
    // detachInterrupt(digitalPinToInterrupt(2));
    // btn1.begin(btn1_change_func);
  }
}

/**
 * mode 1 - Extended Low Mode 
 * dim LEDs flashing, charging LED on, bright LEDs off
 */
void extendedLowMode() {
  highLEDs.off();
  lowLEDs.on();
  // 1 Hz, single 30% DC flash
  totalPeriodLengthinMillis = 1000;
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300/updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis/updatePeriodinMillis;
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      lowLEDs2.set(true);
    } else lowLEDs2.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1 + 1;
  }
}

/**
 * mode 1 - Extended Low Mode 
 * dim LEDs flashing, charging LED on, bright LEDs off
 */
void extendedLowMode2() {
  highLEDs.off();
  lowLEDs.on();
  totalPeriodLengthinMillis = 1000;
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
      lowLEDs2.aSet(sin(0.5 * PI * (ctr1-keyPoints[0]) / (keyPoints[1] - keyPoints[0]))*255);
    } 
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      lowLEDs2.aSet(sin(PI * (0.5 + 0.5 * (ctr1-keyPoints[1]) / (keyPoints[2] - keyPoints[1]))) * 255);
    } 
    else if (ctr1 >= keyPoints[2]) {
      lowLEDs2.aSet(0);
    }
    ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1 + 1;
  }
}

/**
 * mode 2 - Low Mode 
 * dim LEDs on, charging LED on, bright LEDs off
 */
void lowMode() {
  lowLEDs.on();
  lowLEDs2.on();
  highLEDs.off();
}

/**
 * mode 3 - High Mode 
 * dim LEDs on, charging LED on, bright LEDs on
 */
void highMode() {
  lowLEDs.on();
  highLEDs.on();
}

/**
 * mode 4 - Flashing Mode 
 * dim LEDs on, charging LED on, bright LEDs flashing
 */
void flashingMode() {
  totalPeriodLengthinMillis = 1000;
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300/updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis/updatePeriodinMillis;
  lowLEDs.on();
  lowLEDs2.on();
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      highLEDs.set(true);
    } 
    else highLEDs.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1 + 1;
  }
}

/**
 * mode 5 - Fading Mode 
 * dim LEDs on, charging LED on, bright LEDs fading
 */
void fadingMode() {
  totalPeriodLengthinMillis = 1000;
  lowLEDs.on();
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
    } 
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2]) {
      highLEDs.aSet(sin(PI * (0.5 + 0.5 * (ctr1-keyPoints[1]) / (keyPoints[2] - keyPoints[1]))) * 255);
    } 
    else if (ctr1 >= keyPoints[2]) {
      highLEDs.aSet(0);
    }
    ctr1 = ctr1 > keyPoints[3] - 1? 0:ctr1 + 1;
  }
}

void dynamoFlash() {
  totalPeriodLengthinMillis = 110;
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 1;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 43;
  keyPoints[2] = totalPeriodLengthinMillis;
  lowLEDs.on();
  lowLEDs2.on();
  if (millis() - flashCycleTimer >= updatePeriodinMillis) {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1]) {
      highLEDs.set(true);
    } 
    else highLEDs.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1? 0:ctr1 + 1;
  }
}

void setup() {
  wdt_reset();
  wdt_disable();
  if (debug) Serial.begin(115200);
  Serial.println("start");
  btn1.begin(btn1_change_func);
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  btn1.set2ShortPressFunc(btn1_2shortclick_func);
  btn1.set1LongPressFunc(btn1_1longclick_func);
  lowLEDs.begin();
  lowLEDs2.begin();
  highLEDs.begin();
  analogReference(INTERNAL);
}

void loop() {
  btn1.loop();
  lowLEDs.loop();
  lowLEDs2.loop();
  highLEDs.loop();
  
  switch (curLightMode) {
    case 1:
      extendedLowMode();
      break;
    case 2:
      extendedLowMode2();
      break;
    case 3:
      lowMode();
      break;
    case 4:
      highMode();
      break;
    case 5:
      flashingMode();
      break;
    case 6:
      fadingMode();
      break;
    case 7:
      dynamoFlash();
      break;
    default: 
      offMode();
  }
  checkBatVolts(); 
  // updateChargeLED();
  // checkAutoMode();

  if (debug) {
    if (millis() - lastTimePrinted > 10) {
      lastTimePrinted = millis();
      // Serial.print(analogRead(chargingPin));
      // Serial.print(", ");
      // Serial.print(analogRead(batPin));
      // Serial.print(chargingPinVolts);
      // Serial.print(", ");
      // Serial.print(batVolts);
      // Serial.print("ischarging=");
      // Serial.print(isCharging);
      // Serial.print("mode=");
      // Serial.print(curLightMode);
      // Serial.println(); 
    }
  }
  
}


// #include <avr/sleep.h>
// #include <avr/power.h>
// #include <Arduino.h> 

// void setup () 
// {
//   set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
//   noInterrupts ();           // timed sequence follows
//   sleep_enable();
 
//   ADCSRA = 0;
//   // turn off brown-out enable in software
//   MCUCR = bit (BODS) | bit (BODSE);  // turn on brown-out enable select
//   MCUCR = bit (BODS);        // this must be done within 4 clock cycles of above
//   interrupts ();             // guarantees next instruction executed
//   sleep_cpu ();              // sleep within 3 clock cycles of above
// }  // end of setup

// void loop () { }


/**
 * When the bike light is in Off Mode, 
 *  if the battery is charging, turn on the charging LED, 
 *  else turn off the charging LED. 
 */
// void updateChargeLED() {
//   if (!isSleeping && millis() - lastTimeBtnDoubleClicked < 1000) {
//     return;
//   }
//   chargingPinReading = analogRead(chargingPin);
//   chargingPinVolts = chargingPinReading*1.1/1023.0*6;
//   if (chargingPinVolts > chargingThresholdVolts || curLightMode > 0) { 
//     chargingLEDs.on();
//   } else {
//     chargingLEDs.off();
//   }
//   // charging pin value debouncing
//   // 
//   if (chargingPinVolts <= chargingThresholdVolts) {
//     lastTimeChargingVoltsExceeded = millis();
//   }
//   if (millis() - lastTimeChargingVoltsExceeded > 500 && chargingPinVolts > chargingThresholdVolts) {
//     isCharging = true;
//   }
//   else {
//     isCharging = false;
//   }
//   // Serial.print(chargingPinVolts);
//   // Serial.print(", ");
//   // Serial.print(chargingThresholdVolts);
//   // Serial.print(", ");
  
// }