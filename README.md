# Arduino Solar Rear Bicycle Light 
## Main Hardware Components
- Arduino Nano
- 6V 1W solar panel 
- TP4056 li-ion charger with BMS chip 
- 18650 battery holder with 18650 battery x1  
- 10k resistor (solar panel voltage divider)
- 50k resistor (solar panel voltage divider) 
- Diffused red LED x5 (dim light, one as a charging indicator when turned off)
- 105R resistor x5
- Clear red LED x4 (bright light) 
- 70R resistor x4
- 2N3904 NPN transistor x2
- 10k resistor x2
- Momentary pushbutton 

## Things to calculate/ measure: 
- solar panel no-load voltage 
- solar panel max current
- expected battery charge time 
- expected runtime 
- voltage divider resistances 

assume 6V max voltage input, 5V max voltage output 
6 * r2 / (r1 + r2) = 5 
6 * r2 = 5 * (r1 + r2)
6 * r2 = 5 * r1 + 5 * r2 
r2 = 5 * r1 
The current through the voltage divider must be minimal. 
if r2 is 50k, r1 is 10k.

- LED current limiting resistors 
    - diffused LED 

Li-ion voltage max 4.2V 
LED current = 20mA
(4.2-2.1)/0.02 = 105 R resistor minimum

    - clear LED

Li-ion voltage max 4.2V 
LED current = 30mA
(4.2-2.1)/0.03 = 70 R resistor minimum

- power consumption 
    - power consumption in Off Mode (quiescent current)
    - power consumption in Low Mode 
    - power consumption in High Mode 
- 
