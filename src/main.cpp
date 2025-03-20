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

#define EXTENDED_LOW_MODE 1
#define EXTENDED_LOW_MODE_2 2
#define LOW_MODE 3
#define HIGH_MODE 4
#define HIGH_FLASHING_MODE 5
#define HIGH_FADING_MODE 6
#define DYNAMO_FLASH_MODE 7
#define OFF_MODE 0

#define NO_MODE_MEMORY 1
#define MODE_MEMORY 2
#define MODE_MEMORY_AUTO_ON 3

bool indicating = false;
struct Config
{
  bool isOn;
  byte curLightMode;
  byte curMMMode; // current mode memory mode
  bool autoMode;
};

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

bool sequence[10];
int sequenceLen = 0;
int curSequencePos = 0;
unsigned long lt1;

const int NUM_LIGHT_MODES = 7;
Config config;
Config savedConfig;
int chargingPinReading;
double chargingPinVolts, batVolts;
bool lowBattery, deadBattery;
// bool isCharging;
// bool isSleeping;

// update period for fading modes
unsigned int updatePeriodinMillis = 5;
// period length for flashing and fading modes
int totalPeriodLengthinMillis = 1000;
// keypoints per mode cycle
unsigned int keyPoints[4];
// flashCycleTimer - used to keep time
unsigned long flashCycleTimer;
unsigned long lastTimeWoken;
unsigned int longPressTime;
unsigned long lastTimeBtnPressed;
unsigned long lastTimeLongPressChecked;
unsigned long lastTimeValuesChanged;
bool configSaved = true;
bool wokenUp;

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

void startWakeUpTimer()
{
  lastTimeWoken = millis();
  lastTimeLongPressChecked = millis();
  lastTimeBtnPressed = millis();
}

void saveConfig()
{
  EEPROM.put(20, config);
  Serial.println("config saved");
}

void loadConfig()
{
  EEPROM.get(20, config);
  delay(20);
  // validate config values
  if (config.curLightMode < 1 || config.curLightMode > NUM_LIGHT_MODES)
    config.curLightMode = 1;
  if (config.curMMMode < 1 || config.curMMMode > 3)
    config.curMMMode = 1;
  // switch behavior based on the mode memory mode
  switch (config.curMMMode)
  {
  case NO_MODE_MEMORY:
    config.isOn = false;
    config.curLightMode = 1;
    break;
  // case MODE_MEMORY:
  //   break;
  case MODE_MEMORY_AUTO_ON:
    config.isOn = true;
    break;
  }
  // Serial.print("isOn ");
  // Serial.println(config.isOn);
  Serial.println("config loaded");
}

void checkBatVolts()
{
  batVolts = analogRead(batPin) * 1.1 / 1023.0 * 6;
  // turn off the light if the battery voltage is waaay too low
  if (batVolts <= deadBatVolts)
  {
    deadBatCountDown++;
    lowBatCountDown = 0;
    if (deadBatCountDown > 10)
    {
      deadBatCountDown = 0;
      config.curLightMode = 0;
      lowBattery = true;
      deadBattery = true;
    }
  }
  // turn on the light in extended low mode if battery is low and the light is turned on
  else if (batVolts > deadBatVolts && batVolts <= lowBatVolts)
  {
    deadBatCountDown = 0;
    lowBatCountDown++;
    if (lowBatCountDown > 10)
    {
      lowBatCountDown = 0;
      lowBattery = true;
      deadBattery = false;
    }
  }
  else
  {
    deadBatCountDown = 0;
    lowBatCountDown = 0;
    lowBattery = false;
    deadBattery = false;
  }
  if (config.curLightMode > 1 && lowBattery)
  {
    config.curLightMode = 1;
  }

  // if the battery is low and the current mode is greater than extended low mode,
  // set current mode to off mode.
  if (deadBattery)
  {
    config.curLightMode = 0;
  }

  // (lowBattery && config.curLightMode >= 1) ||
}

void offAllLEDs()
{
  lowLEDs.off();
  lowLEDs2.off();
  highLEDs.off();
}

void generateLEDIndication(int num)
{
  curSequencePos = 0;
  sequenceLen = 10;
  for (int i = 0; i < sequenceLen; i++)
  {
    sequence[i] = 0;
  }
  for (int i = 0; i < num; i++)
  {
    sequence[i * 2] = 1;
  }
}

void indicateLED()
{
  indicating = true;
  lt1 = millis();
  offAllLEDs();
}

/**
 * single click - switch and cycle through modes
 */
void btn1_1shortclick_func()
{
  Serial.println("1 short click");
  lastTimeBtnPressed = millis();
  lastTimeValuesChanged = millis();
  configSaved = false;

  // lastTimeBtnClicked = millis();
  // if auto mode is turned on and the light is charging,
  // set current mode to saved mode.
  // if (config.autoMode && isCharging) {
  //   config.curLightMode = savedMode;
  // }

  // // if the battery voltage is lower than the defined low battery
  // // voltage plus a certain buffer value, it is considered battery low
  // // when the button is pressed.
  // if (batVolts < lowBatVolts + hysteresisBatVolts) {
  //   lowBattery = true;
  // }
  if (config.isOn)
  {
    config.curLightMode++;
  }

  // Cycle current mode to the start when it exceeds the last mode.
  if (config.curLightMode > NUM_LIGHT_MODES)
  {
    config.curLightMode = 1;
  }

  // if auto mode is turned on,
  // set saved mode to current mode.
  // if (config.autoMode) {
  //   savedMode = config.curLightMode;
  // }
  if (debug)
  {
    Serial.print("config.curLightMode = ");
    Serial.println(config.curLightMode);
  }
  // reset watchdog timer
  wdt_reset();
}

/**
 * double click - toggle config.autoMode
 */
void btn1_2shortclick_func()
{
  Serial.println("2 short click");
  lastTimeBtnPressed = millis();
  lastTimeValuesChanged = millis();
  configSaved = false;
  // lastTimeBtnDoubleClicked = millis();
  // lastTimeBtnClicked = millis();

  if (config.isOn)
  {
    config.curMMMode++;
  }
  if (config.curMMMode > 3)
  {
    // config.autoMode = !config.autoMode;
    config.curMMMode = 1;
  }
  generateLEDIndication(config.curMMMode);
  indicateLED();

  // lowLEDs2.setLoopUnitDuration(200);
  // if (config.autoMode) {
  //   bool loopSeq[] = {0,1,0,1,0};
  //   lowLEDs2.startTimer(1000, true);
  //   lowLEDs2.setLoopSequence(loopSeq, 5);
  //   lowLEDs2.startLoop();
  // } else {
  //   bool loopSeq[] = {0,1,0,0,0};
  //   lowLEDs2.startTimer(1000, true);
  //   lowLEDs2.setLoopSequence(loopSeq, 5);
  //   lowLEDs2.startLoop();
  // }
  if (debug)
  {
    Serial.print("config.curMMMode=");
    Serial.println(config.curMMMode);
    // Serial.print("mode=");
    // Serial.print(config.curLightMode);
    // Serial.println();
    // Serial.print("config.autoMode=");
    // Serial.println(config.autoMode);
    // Serial.print("mode=");
    // Serial.print(config.curLightMode);
    // Serial.println();
  }
  // reset watchdog timer
  wdt_reset();
}

void btn1_1longclick_func()
{
  Serial.println("1 long click");
  config.isOn = !config.isOn;
  lastTimeValuesChanged = millis();
  configSaved = false;
  lastTimeBtnPressed = millis();
}

// ISR for interrupt button library
void btn1_change_func()
{
  btn1.changeInterruptFunc();
}

// ISR for waking up from deep sleep
void wakeupISR()
{
  ADCSRA |= 1 << ADEN;
  startWakeUpTimer();
  wokenUp = true;
  btn1.begin(btn1_change_func);
}

// ISR triggered by watchdog timer every 1 seconds during deep sleep,
// before going back to sleep.
ISR(WDT_vect)
{
  ADCSRA |= 1 << ADEN;
  wdt_reset();
  checkBatVolts();
}

void checkWakeupLongPressLoop()
{
  if (wokenUp)
  {
    // Serial.print("wk ");
    // Serial.print(longPressTime);
    // Serial.print(" btn ");
    // Serial.println(btn1.getState());
    if (!btn1.getState())
    {
      longPressTime += millis() - lastTimeLongPressChecked;
      lastTimeLongPressChecked = millis();
    }
    if (longPressTime > 800)
    {
      config.isOn = true;
      lastTimeValuesChanged = millis();
      configSaved = false;
      longPressTime = 0;
      wokenUp = false;
    }
    if (millis() - lastTimeWoken > 2000)
    {
      wokenUp = false;
    }
    // Serial.println("wakeupcheck");
    // if button is long pressed, turn on the lights.
  }
}

void autosaveConfig()
{
  if (millis() - lastTimeValuesChanged > 5000 && !configSaved)
  {
    saveConfig();
    Serial.println("saved eeprom config");
    configSaved = true;
  }
}

/**

If the light is in config.autoMode {
  If the light is charging {
    If the saved mode is different from the current mode {
      Save the current mode
      // Set current mode to zero
    }
    If the button is pressed {
      Set current mode to the saved mode
      Increment the current mode in a cycle
      Start a timer lightsTimer that keeps the lights on five seconds
        before it turns off due to config.autoMode and charging.
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
Else if the light is not in config.autoMode {
  Maintain the current mode
  If the saved mode is nonzero and the current mode is zero {
    Resume the saved mode
  }
}

*/

// void checkAutoMode() {
//   if (!deadBattery) {
//     if (config.autoMode) {
//       // Serial.print("config.curLightMode = ");
//       // Serial.print(config.curLightMode);
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
//           if (config.curLightMode) {
//             savedMode = config.curLightMode;
//             config.curLightMode = 0;
//           }
//         }
//       }
//       else {
//         if (config.curLightMode != savedMode && savedMode) {
//           config.curLightMode = savedMode;
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

/**
 * mode 0 - Off Mode
 */
void offMode()
{
  offAllLEDs();
  // Serial.println("off");
  if (millis() - lastTimeBtnPressed > 10000)
  {
    longPressTime = 0;
    // isSleeping = true;
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
    MCUCR = bit(BODS) | bit(BODSE); // turn on brown-out enable select
    MCUCR = bit(BODS);              // this must be done within 4 clock cycles of above
    interrupts();                   // guarantees next instruction executed

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
void extendedLowMode()
{
  highLEDs.off();
  lowLEDs.on();
  // 1 Hz, single 30% DC flash
  totalPeriodLengthinMillis = 1000;
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300 / updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis / updatePeriodinMillis;
  if (millis() - flashCycleTimer >= updatePeriodinMillis)
  {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1])
    {
      lowLEDs2.set(true);
    }
    else
      lowLEDs2.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1 ? 0 : ctr1 + 1;
  }
}

/**
 * mode 1 - Extended Low Mode
 * dim LEDs flashing, charging LED on, bright LEDs off
 */
void extendedLowMode2()
{
  highLEDs.off();
  lowLEDs.on();
  totalPeriodLengthinMillis = 1000;
  updatePeriodinMillis = 5;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 400 / updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 400 / updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 200 / updatePeriodinMillis;
  // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,80,160,200
  if (millis() - flashCycleTimer >= updatePeriodinMillis)
  {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1])
    {
      lowLEDs2.aSet(sin(0.5 * PI * (ctr1 - keyPoints[0]) / (keyPoints[1] - keyPoints[0])) * 255);
    }
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2])
    {
      lowLEDs2.aSet(sin(PI * (0.5 + 0.5 * (ctr1 - keyPoints[1]) / (keyPoints[2] - keyPoints[1]))) * 255);
    }
    else if (ctr1 >= keyPoints[2])
    {
      lowLEDs2.aSet(0);
    }
    ctr1 = ctr1 > keyPoints[3] - 1 ? 0 : ctr1 + 1;
  }
}

/**
 * mode 2 - Low Mode
 * dim LEDs on, charging LED on, bright LEDs off
 */
void lowMode()
{
  lowLEDs.on();
  lowLEDs2.on();
  highLEDs.off();
}

/**
 * mode 3 - High Mode
 * dim LEDs on, charging LED on, bright LEDs on
 */
void highMode()
{
  lowLEDs.on();
  lowLEDs2.on();
  highLEDs.on();
}

void highFlashMode()
{
}

/**
 * mode 4 - Flashing Mode
 * dim LEDs on, charging LED on, bright LEDs flashing
 */
void flashingMode()
{
  totalPeriodLengthinMillis = 1000;
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 100;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 300 / updatePeriodinMillis;
  keyPoints[2] = totalPeriodLengthinMillis / updatePeriodinMillis;
  lowLEDs.on();
  lowLEDs2.on();
  if (millis() - flashCycleTimer >= updatePeriodinMillis)
  {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1])
    {
      highLEDs.set(true);
    }
    else
      highLEDs.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1 ? 0 : ctr1 + 1;
  }
}

/**
 * mode 5 - Fading Mode
 * dim LEDs on, charging LED on, bright LEDs fading
 */
void fadingMode()
{
  totalPeriodLengthinMillis = 1000;
  updatePeriodinMillis = 5;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 400 / updatePeriodinMillis;
  keyPoints[2] = keyPoints[1] + 400 / updatePeriodinMillis;
  keyPoints[3] = keyPoints[2] + 200 / updatePeriodinMillis;
  lowLEDs.on();
  lowLEDs2.on();
  // 1 Hz fade; 400 mS rise, 400 mS fall, 200 mS off
  // 5 ms fading steps
  // 200 total steps; 0,80,160,200
  if (millis() - flashCycleTimer >= updatePeriodinMillis)
  {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1])
    {
      highLEDs.aSet(sin(0.5 * PI * (ctr1 - keyPoints[0]) / (keyPoints[1] - keyPoints[0])) * 255);
    }
    else if (ctr1 >= keyPoints[1] && ctr1 < keyPoints[2])
    {
      highLEDs.aSet(sin(PI * (0.5 + 0.5 * (ctr1 - keyPoints[1]) / (keyPoints[2] - keyPoints[1]))) * 255);
    }
    else if (ctr1 >= keyPoints[2])
    {
      highLEDs.aSet(0);
    }
    ctr1 = ctr1 > keyPoints[3] - 1 ? 0 : ctr1 + 1;
  }
}

void dynamoFlash()
{
  totalPeriodLengthinMillis = 110;
  // 1 Hz, single 30% DC flash
  updatePeriodinMillis = 1;
  keyPoints[0] = 0;
  keyPoints[1] = keyPoints[0] + 43;
  keyPoints[2] = totalPeriodLengthinMillis;
  lowLEDs.on();
  lowLEDs2.on();
  if (millis() - flashCycleTimer >= updatePeriodinMillis)
  {
    flashCycleTimer = millis();
    if (ctr1 < keyPoints[1])
    {
      highLEDs.set(true);
    }
    else
      highLEDs.set(false);
    ctr1 = ctr1 > keyPoints[2] - 1 ? 0 : ctr1 + 1;
  }
}

void indicateLEDLoop()
{
  if (indicating)
  {
    if (millis() - lt1 > 100)
    {
      lt1 = millis();
      lowLEDs.set(sequence[curSequencePos]);
      curSequencePos++;
      if (curSequencePos > sequenceLen - 1)
      {
        indicating = false;
      }
    }
  }
}

void setup()
{
  wdt_reset();
  wdt_disable();
  if (debug)
    Serial.begin(115200);
  Serial.println("start");
  btn1.begin(btn1_change_func);
  btn1.set1ShortPressFunc(btn1_1shortclick_func);
  btn1.set2ShortPressFunc(btn1_2shortclick_func);
  btn1.set1LongPressFunc(btn1_1longclick_func);
  lowLEDs.begin();
  lowLEDs2.begin();
  highLEDs.begin();
  analogReference(INTERNAL);
  loadConfig();
}

void loop()
{
  btn1.loop();
  lowLEDs.loop();
  lowLEDs2.loop();
  highLEDs.loop();

  indicateLEDLoop();
  checkWakeupLongPressLoop();
  if (config.isOn && !indicating)
  {
    switch (config.curLightMode)
    {
    case EXTENDED_LOW_MODE_2:
      extendedLowMode2();
      break;
    case LOW_MODE:
      lowMode();
      break;
    case HIGH_MODE:
      highMode();
      break;
    case HIGH_FLASHING_MODE:
      flashingMode();
      break;
    case HIGH_FADING_MODE:
      fadingMode();
      break;
    case DYNAMO_FLASH_MODE:
      dynamoFlash();
      break;
    default:
      config.curLightMode = EXTENDED_LOW_MODE;
      extendedLowMode();
    }
  }
  else if (!config.isOn)
  {
    offMode();
  }

  checkBatVolts();
  // updateChargeLED();
  // checkAutoMode();

  if (debug)
  {
    if (millis() - lastTimePrinted > 10)
    {
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
      // Serial.print(config.curLightMode);
      // Serial.println();
    }
  }
  autosaveConfig();
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
//   if (chargingPinVolts > chargingThresholdVolts || config.curLightMode > 0) {
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