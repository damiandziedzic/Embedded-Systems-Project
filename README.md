# Embedded-Systems-Project
Drive three wheeled robot wirelessly via hand motions.
NOTE: Both microcontrollers use the Atmel328P, which is on the Arduino Uno
Sensors used (free sample of just sensors):
http://ams.com/eng/Products/Magnetic-Position-Sensors/Linear-Position/AS5304

Full development board can be purchased at digikey with the magnet:
https://www.digikey.com/product-detail/en/ams/AS5304-TS_EK_AB/AS5304-AB-ND/3828351

## Sensor Microcontroller
* Connect RF24 module via SPI bus, but use PIN 7, 8 for CE and CSN
* The module must be powered by 3.3V
* Two magnetic linear encoders are used. One for speed, and the other for direction (both powered by 5V).
* Speed sensor: Output A -> Pin 2, Output B -> Pin 9
* Direction sensor: Output A -> Pin 3, Output B -> Pin 6
* Pull speed sensor magnet to speed up, and push to slow down until reaching full stop.
* Move direction magnet to the right to turn right, move to the left to turn left.

## Robot Microcontroller
* Timer1 was used for PWM signal. The code assumes motors are wired such that wheels move in opposite directions if same PWM signal is applied to both motors.
* Connect RF24 module the same way as mentioned above.
* PWM output is on PIN 9, 10.
