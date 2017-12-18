// This code receives the data sent from sensors, scales it, and applies PWM to motors.
// NOTE: for this code to work, it assumes that the wheels move in opposite direction.
// If robot is flipped upside down, the wires from the driver boards to the wheels should be:
// BLACK | RED | BLACK | RED (in order from left to right)

#include <SPI.h>
#include "RF24.h"

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 0;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7, 8);
/**********************************************************/

byte addresses[][6] = {"ABCD", "EFGH"};


void setup() {
  Serial.begin(115200);

  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);

  // Open a writing and reading pipe on each radio, with opposite addresses
  if (radioNumber) {
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1, addresses[0]);
  } else {
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1, addresses[1]);
  }

  // Start the radio listening for data
  radio.startListening();


  // Pin Setup
  DDRB |= (1 << PB1) | (1 << PB2);

  // PWM Setup
  // CW   -> 1520us - 2000us // For each mode this is the time where the PWM signal is HIGH, so duty cycle
  // CCW  -> 1000us - 1480us
  // Stop -> 1480us - 1520us
  // Max time high needed is 2000us -> 500 Hz
  // If use CLK/64 for 9-bit timer on TIMER1, then top is 511 and after 511 ticks you have 2044us (WGM1: 0110)
  // Our max time high is 2000us which is 97.85% duty cycle of 2044us time total
  TCCR1A = 0b10100010; // Fast PWM, non-inverting, WGM1[1:0]
  TCCR1B = 0b00001011; // WGM1[3:2], CLK/64 pre-scaler
  
 // OCR1A/B * 4 = corresponding time HIGH in usec
 // Set to complete stop in setup
  OCR1A = 375;
  OCR1B = 375;
}

// Recieved data array
unsigned long data[2];
// data[0] -> Speed Increment from the base value below
// data[1] -> Direction

// Base values, 375 is full stop
unsigned long base_val = 375;

void loop() {

  if ( radio.available()) {

    while (radio.available()) {                                   // While there is data ready
      radio.read( &data, sizeof(data) );                          // Get the payload
    }

    char response[3] = "ACK";                                     // Acknowledge response for the sensor ouput
    radio.stopListening();                                        // First, stop listening so we can talk
    radio.write( &response, sizeof(unsigned long) );              // Send the final one back.

    radio.startListening();                                       // Now, resume listening so we catch the next packets.

    unsigned long change = data[1];                               // New direction 
    long leftOrRight= 25-change;                                  // If negative left wheel faster, if postive right wheel faster 
    unsigned long turnValue = abs(leftOrRight);                   // change scale to effect speed 
    unsigned long speed_mapped = map(data[0], 0, 30, 0, 125);     // USE THIS FOR MAGNETIC SENSOR

    //OCR1A --> Right wheel NEEDS TO BE SUBTRACTED FROM BASE TO SPEED UP -> do no exceed bounds of min: 250, max: 375
    //OCR1B --> Left wheel NEEDS TO BE ADDED TO BASE TO SPEED UP -> do not exceed bounds of min: 375, max: 500

    if (speed_mapped == 0){ // Make sure to stop if speed_mapped is zero
      OCR1A = base_val;
      OCR1B = base_val;
    }
    else{ // Otherwise scale values
      if (leftOrRight > 0) {  // right wheel faster so turn left
        OCR1A = constrain(base_val - speed_mapped - turnValue, 250, 375); // right wheel goes faster
        OCR1B = constrain(base_val + speed_mapped - turnValue, 375, 500); // left wheel goes slower
//        OCR1B = constrain(base_val + speed_mapped, 375, 500);
      }
      else if (leftOrRight < 0) { //right faster 
        OCR1A = constrain(base_val - speed_mapped + turnValue, 250, 375); // right wheel goes slower
//        OCR1A = constrain(base_val - speed_mapped, 250, 375); 
        OCR1B = constrain(base_val + speed_mapped + turnValue, 375, 500); // left wheel goes faster
      }
      else {  // same speed 
        OCR1A = base_val - speed_mapped;
        OCR1B = base_val + speed_mapped;
      }
    }
    
    // Try the following tunings to improve driving performance.
    // NOTE: Check if there is better perfomance if only one wheel is sped up instead of also slowing down the opposite wheel.
    // NOTE: Maybe try different scalings of turnValue.

    Serial.print(F("Got Sensor Output1 "));
    Serial.print(data[0]);
    Serial.print(", ");
    Serial.print(F("Got Sensor Output2 "));
    Serial.print(data[1]);
    Serial.print(F(", Sent response "));
    Serial.println(response);
  }
}
