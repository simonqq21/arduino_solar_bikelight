# Arduino Solar Rear Bicycle Light 
## Main Hardware Components
- Arduino Nano
- 6V 1W solar panel 
- TP4056 li-ion charger with BMS chip 
- 18650 battery holder with 18650 battery x1  
- 10k resistor (solar panel voltage divider)
- 50k resistor (solar panel voltage divider) 
- Diffused red LED x5 (dim light, one as a charging indicator when turned off)
- 150R resistor x5
- Clear red LED x4 (bright light) 
- 150R resistor x4
- 2N3904 NPN transistor x2
- 10k resistor x2
- Momentary pushbutton 

## Things to calculate/ measure: 
solar panel no-load voltage 
solar panel max current
expected battery charge time 
expected runtime 
voltage divider resistances 

assume 6V max voltage input, 5V max voltage output 
6 * r2 / (r1 + r2) = 5 
6 * r2 = 5 * (r1 + r2)
6 * r2 = 5 * r1 + 5 * r2 
r2 = 5 * r1 
The current through the voltage divider must be minimal. 
if r2 is 50k, r1 is 10k.

LED current limiting resistors 
diffused LED 

Li-ion voltage max 4.2V 
LED current = 20mA
(4.2-2.1)/0.02 = 105 R resistor minimum

clear LED

Li-ion voltage max 4.2V 
LED current = 30mA
(4.2-2.1)/0.03 = 70 R resistor minimum

Calculations for the voltage divider resistances when using the internal 1.1V Vref 

Min input (PV panel and USB charger) voltage: 0V
Max input (PV panel and USB charger) voltage: 7V
max USB charger voltage: 5V
max PV panel voltage: ~7V - 0.3V = 6.7V (0.3V voltage drop from the diode)
Max input (PV panel and USB charger) voltage: 6.7 ~= 7V
Vin = 7V 
Vout = 1.1V
7* r2/(r1+r2) = 1.1
7r2 = 1.1r1 + 1.1r2 
5.9r2 = 1.1r1 
r1 = 5.9r2/1.1
r2 = 10k 
r1 = 53636.364 ~= 53k 

Min battery voltage: 3.0V
Max battery voltage: 4.2V
Vin = 4.2V
Vout = 1.1V
4.2* r2/(r1+r2) = 1.1 
4.2r2 = 1.1r1 + 1.1r2 
3.1r2 = 1.1r1
r1 = 3.1r2 / 1.1 
r2 = 10k  
r1 = 28181.818 ~= 28k 

# Power Consumption 
power consumption in Off Mode (quiescent current) 35uA
power consumption in Low Mode 78mA
power consumption in High Mode 118mA
