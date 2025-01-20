#include "Arduino.h"
#include "LEDlib.h" 

LED::LED(int pin) {
    _pin = pin;
}

void LED::begin() {
    pinMode(_pin, OUTPUT);
}

void LED::loop() {
    if (_timerOn && millis() - _lastTimeTimerSet >= _onDuration) {
        _ledMode = LED_OFF;
        _timerOn = false;
        _ledASet = false;
        _statusMode = false;
        if (_resumePrevLEDMode) {
            _ledMode = _previousLEDMode;
        }
    }

    switch (_ledMode)
    {
    case LED_ON:
        _nextLEDDigitalVal = true;
        break;
    case LED_BLINK:
        if (millis() - _previousMillis > _blinkOnPeriod && _curLEDDigitalVal == true) {
            _previousMillis = millis();
            _nextLEDDigitalVal = false;

        }
        else if (millis() - _previousMillis > _blinkOffPeriod && _curLEDDigitalVal == false) {
            _previousMillis = millis();
            _nextLEDDigitalVal = true;
        }
        break;
    case LED_ANALOGSET:
        if (!_ledASet) {
            analogWrite(_pin, _curLEDAnalogVal);
            _ledASet = true;
        }
        break;
    case LED_LOOP:
        if (millis() - _previousMillis > _loopUnitDuration) {
            _previousMillis = millis();
            _nextLEDDigitalVal = _loopSequence[_curLoopSequencePos];
            // Serial.printf("step %d\n", _loopSequence[_curLoopSequencePos]);
            // Serial.printf("%d, %d\n", _nextLEDDigitalVal, _curLEDDigitalVal);
            if (_curLoopSequencePos < _loopSequenceLength-1) {
                _curLoopSequencePos++;
            } else {
                _curLoopSequencePos = 0;
            }
        }
        break;
    default:
        _nextLEDDigitalVal = false;
    }

    // write values to the LED pin only upon change to avoid flickering
    if (_ledMode != LED_ANALOGSET) {
        if (_curLEDDigitalVal != _nextLEDDigitalVal) {
            _curLEDDigitalVal = _nextLEDDigitalVal;
            digitalWrite(_pin, _curLEDDigitalVal);
        }
    }
}

void LED::on() {
    // Serial.printf("statusMode=%d\n", _statusMode);
    if (!_statusMode) {
        pinMode(_pin, OUTPUT);
        _ledMode = LED_ON;
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::off() {
    if (!_statusMode) {
        pinMode(_pin, OUTPUT);
        _ledMode = LED_OFF;
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::toggle() {
    if (!_statusMode) {
        switch (_ledMode)
        {
        case LED_OFF:
            this->on();
            break;
        
        default:
            this->off();
            break;
        }
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::set(bool state) {
    if (!_statusMode) {
        if (state)
            this->on();
        else 
            this->off();
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::blink(unsigned int period, double dutyCycle) {
    if (!_statusMode) {
        pinMode(_pin, OUTPUT);
        _ledMode = LED_BLINK;
        if (dutyCycle > 1.0) {
            dutyCycle = 1.0;
        }
        else if (dutyCycle < 0.0) {
            dutyCycle = 0.0;
        }
        _blinkOnPeriod = period * dutyCycle;
        _blinkOffPeriod = period * (1.0-dutyCycle);
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::aSet(int aValue) {
    if (!_statusMode) {
        _ledMode = LED_ANALOGSET;
        _curLEDAnalogVal = aValue;
        _ledASet = false;
        if (_timerOn) {
            _statusMode = true;
        }
    }
}

void LED::startTimer(int milliseconds, bool resumePreviousMode) {
    _previousLEDMode = _ledMode;
    _resumePrevLEDMode = resumePreviousMode;

    if (milliseconds > 0) {
        _timerOn = true;
        _onDuration = milliseconds;
        _lastTimeTimerSet = millis();
    }
    else {
        _timerOn = false;
    }
}

void LED::setLoopSequence(bool loopSequence[], unsigned int loopSequenceLength) {
    _loopSequenceLength = loopSequenceLength;
    for (int i=0;i< _loopSequenceLength;i++) {
        _loopSequence[i] = loopSequence[i];
    }
}
void LED::setLoopUnitDuration(unsigned int LoopUnitDuration) {
    _loopUnitDuration = LoopUnitDuration;
}
void LED::startLoop() {
    _ledMode = LED_LOOP;
    _curLoopSequencePos = 0;
}