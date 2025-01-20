#include <Arduino.h>
#include "math.h"
#include "buttonlib2.h"
#include "LEDlib.h"
#include <avr/sleep.h>  
#include <avr/wdt.h> 

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
unsigned long powerOnTime; 

/**
 * ctr1 is a counter variable used to animate the LEDs.
 * ctr1 is measured in multiples of updatePeriodinMillis milliseconds, which varies 
 * depending on the mode. For example, it could be in multiples of 5 ms for fading, or 100 ms
 * for flashing.
 */
unsigned int ctr1;
unsigned long lastTimePrinted = 0;
bool debug = true;

void startSleepTimer() {
  powerOnTime = millis();
}

/**
 * single click - switch and cycle through modes
 */
void btn1_1shortclick_func() {
  startSleepTimer();
  curMode++;
  // Serial.println(configuration->curBrightness > PWR_HIGH);
  if (curMode > 4) curMode = 0;
  if (debug) {
    Serial.print("curMode = ");
    Serial.println(curMode);
  }
  // reset watchdog timer
  wdt_reset();
}

// ISR for interrupt button library 
void btn1_change_func() {
  btn1.changeInterruptFunc();
}

// ISR for waking up from deep sleep
void wakeupISR() {
  startSleepTimer(); 
  btn1.begin(btn1_change_func);
  curMode = 1;
  // reset button temporarily to prevent double trigger
  btn1.reset();
}

/**
 * When the bike light is in Off Mode, 
 *  if the battery is charging, turn on the charging LED, 
 *  else turn off the charging LED. 
 */
void updateChargeLED() {
  chargingPinADCVal = analogRead(chargingPin);
  if (chargingPinADCVal > 1023*3/4 || curMode > 0) {
    chargingLEDs.on();
  } else {
    chargingLEDs.off();
  }
}

// ISR triggered by watchdog timer every 4 seconds during deep sleep,
// before going back to sleep.
ISR (WDT_vect) {
  wdt_reset();
  updateChargeLED();
}

/**
 * mode 0 - Off Mode
 */
void offMode() {
  lowLEDs.off();
  highLEDs.aSet(0);
  // highLEDs.off();
  if (millis() - powerOnTime > 300) {
    // set interrupt to perform the watchdog ISR every four seconds then go back to sleep
    // clear MCU Status Register
    MCUSR = 0;
    // set watchdog timer change enable and watchdog enable
    WDTCSR = bit (WDCE) | bit (WDE);
    // set WDP3 to WDP0 to trigger watchdog interrupt every 4 seconds and 
    // set WDIE enable watchdog interrupt
    WDTCSR = bit(WDIE) | bit (WDP3) & ~bit (WDP2) & ~bit (WDP1) & ~bit (WDP0);

    // sleep CPU until woken up by the button
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    detachInterrupt(digitalPinToInterrupt(2));
    attachInterrupt(digitalPinToInterrupt(2), wakeupISR, LOW);  
    // sleep_mode(); 
    // sei();
    sleep_cpu();
    // sleep_disable();
    // sei();
    // detachInterrupt(digitalPinToInterrupt(2));
    // btn1.begin(btn1_change_func);
  }
}

/**
 * mode 1 - Low Mode 
 * dim LEDs on, charging LED on, bright LEDs off
 */
void lowMode() {
  lowLEDs.on();
  highLEDs.off();
}

/**
 * mode 1 - Low Mode 
 * dim LEDs on, charging LED on, bright LEDs on
 */
void highMode() {
  lowLEDs.on();
  highLEDs.on();
}

/**
 * mode 1 - Low Mode 
 * dim LEDs on, charging LED on, bright LEDs flashing
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
 * mode 1 - Low Mode 
 * dim LEDs on, charging LED on, bright LEDs fading
 */
void fadingMode() {
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

void setup() {
  wdt_reset();
  wdt_disable();
  if (debug) Serial.begin(115200);
  Serial.println("start");
  btn1.begin(btn1_change_func);
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  lowLEDs.begin();
  chargingLEDs.begin();
  highLEDs.begin();
}

void loop() {
  btn1.loop();
  lowLEDs.loop();
  chargingLEDs.loop();
  highLEDs.loop();
  
  switch (curMode) {
    case 1:
      lowMode();
      break;
    case 2:
      highMode();
      break;
    case 3:
      flashingMode();
      break;
    case 4:
      fadingMode();
      break;
    default: 
      offMode();
  }

  updateChargeLED();

  // if (millis() - lastTimePrinted > 200) {
  //   lastTimePrinted = millis();
  //   // Serial.println(analogRead(chargingPin));
  // }
}

// // **** INCLUDES *****
// #include "LowPower.h"
// #include <Arduino.h>

// // Use pin 2 as wake up pin
// const int wakeUpPin = 2;
// bool sleep;
// int i = 0; 

// void isr1() {
//   Serial.println("isr1");
// }

// void isr0()
// {
//   attachInterrupt(digitalPinToInterrupt(2), isr1, CHANGE);
// }

// void setup()
// {
//     // Configure wake up pin as input.
//     // This will consumes few uA of current.
//     pinMode(wakeUpPin, INPUT_PULLUP);   
//     pinMode(LED_BUILTIN, OUTPUT);
//     // Allow wake up pin to trigger interrupt on low.
//     attachInterrupt(digitalPinToInterrupt(2), isr1, CHANGE);
//     Serial.begin(115200);
// }

// void loop() 
// {
//     // Allow wake up pin to trigger interrupt on low.
//     // attachInterrupt(digitalPinToInterrupt(2), wakeUp, LOW);
    
//     // Enter power down state with ADC and BOD module disabled.
//     // Wake up when wake up pin is low.
    

//     // digitalWrite(LED_BUILTIN, HIGH);
//     // delay(2000);
//     // digitalWrite(LED_BUILTIN, LOW);
//     // delay(2000);
//     // Disable external pin interrupt on wake up pin.
//     // detachInterrupt(digitalPinToInterrupt(2)); 
    
//     // Do something here
//     // Example: Read sensor, data logging, data transmission.
    
//     delay(1000);
//     digitalWrite(LED_BUILTIN, HIGH);
//     delay(500);
//     digitalWrite(LED_BUILTIN, LOW);
//     delay(500);
//     digitalWrite(LED_BUILTIN, HIGH);
//     delay(1000);
//     digitalWrite(LED_BUILTIN, LOW);

//     // detachInterrupt(digitalPinToInterrupt(2));
//     attachInterrupt(digitalPinToInterrupt(2), isr0, LOW); 
//     LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 
// }


      
// int wakePin = 2;                 // pin used for waking up  
// int led=13;  
  
// void wakeUpNow() {  
//   // execute code here after wake-up before returning to the loop() function  
//   // timers and code using timers (serial.print and more...) will not work here.  
//   // we don't really need to execute any special functions here, since we  
//   // just want the thing to wake up  
// }  
  
// void setup() {  
//   Serial.begin(115200);
//   pinMode(wakePin, INPUT_PULLUP);  
//   pinMode(led, OUTPUT);   
//   attachInterrupt(0, wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function wakeUpNow when pin 2 gets LOW  
// }  
  
// void sleepNow() {  
//     set_sleep_mode(SLEEP_MODE_PWR_DOWN);   // sleep mode is set here  
//     sleep_enable();          // enables the sleep bit in the mcucr register  
//     attachInterrupt(0,wakeUpNow, LOW); // use interrupt 0 (pin 2) and run function  
//     sleep_mode();            // here the device is actually put to sleep!!  
//     // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP  
//     sleep_disable();         // first thing after waking from sleep: disable sleep...  
//     detachInterrupt(0);      // disables interrupt 0 on pin 2 so the wakeUpNow code will not be executed during normal running time.  
// }  
  
// void loop() {  
//   digitalWrite(led, HIGH);  
//   delay(1000);  
//   digitalWrite(led, LOW);  
//   sleepNow();     // sleep function called here 
//   Serial.println("ABC");
// }