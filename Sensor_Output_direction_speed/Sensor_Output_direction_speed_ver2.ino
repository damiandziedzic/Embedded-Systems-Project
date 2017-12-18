// Sensor uses Incremental Quadrature AB output
// Output A is counted every falling edge, and Output B is 90 degrees out of phase of A
// So at every falling edge of A, look at B to see if it is high or low. This represents the 
// direction in which the magnetic strip is moving.

// AS5304 sensor resolution is 25um. So one period, or every falling edge represents 0.1mm in distance or one increment/decrement of count.

// Rotary encoder does not show distance resolution in datasheet. But contains 24-pulse encoder with detents.
// Can still use "incrementing count method" in the ISR. But there is no distance index to use. Will need to scale
// rotary sensor differently from AS5304 to get OCR1A/B values.
// NOTE: Rotary encoder (PEC11 Series) was not very stable compared to magnetic sensor

#define distance_index 0.1          // Distance in mm for every interrupt event, use if want to implement distance instead of count number (magnetic sensor)
volatile unsigned long count1 = 0;  // Number of distance counts for sensor 1, also the speed increment
volatile unsigned long count2 = 25; // Number of distance counts for sensor 2, alse the direction value. Start at 25 for halfway point to be straight.

#include <SPI.h>
#include "RF24.h"

/****************** User Config ***************************/
/***      Set this radio as radio number 0 or 1         ***/
bool radioNumber = 1;

/* Hardware configuration: Set up nRF24L01 radio on SPI bus plus pins 7 & 8 */
RF24 radio(7,8);
/**********************************************************/

byte addresses[][6] = {"ABCD","EFGH"};


void setup() {
  Serial.begin(115200);
    
  radio.begin();

  // Set the PA Level low to prevent power supply related issues since this is a
 // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);
  
  // Open a writing and reading pipe on each radio, with opposite addresses
  if(radioNumber){
    radio.openWritingPipe(addresses[1]);
    radio.openReadingPipe(1,addresses[0]);
  }else{
    radio.openWritingPipe(addresses[0]);
    radio.openReadingPipe(1,addresses[1]);
  }
  
  // Start the radio listening for data
  radio.startListening();

  // Set up pins
  DDRB &= 0b11111001;                            // Make sure direction is input for desired pins of port B and D
  DDRD &= 0b10110011;                            
  PORTD |= (1 << PD2) | (1 << PD3) | (1 << PD6); // Pullup resistors for pins using sensors
  PORTB |= (1 << PB1);


  // Setup external interrupts
  EICRA |= 0b00001010;                // Trigger interrupt for both sensors at falling edge
  EIMSK |= (1 << INT1) | (1 << INT0); // Enable both external interrupt pins
  // EIFR is the flag register for both external interrupts

}

// Create array of the two values
unsigned long data[2];

void loop() {
    data[0] = count1;   // Assign sensor values to data array
    data[1] = count2;
  
    radio.stopListening();                                          // First, stop listening so we can talk.
    
    Serial.println(F("Now sending"));
     if (!radio.write( &data, sizeof(data) )){
       Serial.println(F("failed"));
     }

    radio.startListening();                                    // Now, continue listening
    
    unsigned long started_waiting_at = micros();               // Set up a timeout period, get the current microseconds
    boolean timeout = false;                                   // Set up a variable to indicate if a response was received or not
    
    while ( ! radio.available() ){                             // While nothing is received
      if (micros() - started_waiting_at > 200000 ){            // If waited longer than 200ms, indicate timeout and exit while loop
          timeout = true;
          break;
      }      
    }
        
    if ( timeout ){                                             // Describe the results
        Serial.println(F("Failed, response timed out."));
    }else{
        char got_response[3];                                   // Grab the response, compare, and send to debugging spew
        radio.read( &got_response, sizeof(got_response) );
        
        // Spew it
        Serial.print(F("Sent Sensor Output "));
        Serial.print(count1);
        Serial.print(", ");
        Serial.print(count2);
        Serial.print(F(", Got response "));
        Serial.println(got_response);
    }
    
//  For debugging purposes   
  Serial.print(data[0]);
  Serial.print(" / ");
  Serial.println(data[1]);
}

// Interrupt for speed sensor
ISR(INT0_vect){
 static unsigned long last_interrupt_time0 = 0;
 unsigned long interrupt_time0 = millis();
 if (interrupt_time0 - last_interrupt_time0 > 50){ // Debouncing, which waits 100ms to change value
  if(PINB & (1<<PB1)){                              // If sensor 1 direction HIGH
    if (count1 != 30)                               // Do not go above 12 for rotary, 30 for magnetic
     count1++;                                      // Increment sensor 1 counter
  }                                                 
  else if((PINB & (1<<PB1)) == 0){                  // If sensor 1 direction LOW
    if(count1 != 0)                                 // Do no go below 0
     count1--;                                      // Decrement sensor 1 counter
  }                                              
 last_interrupt_time0 = interrupt_time0;
 }
}


// Had to use a different pin to read direction pin to not interfere with wireless module
// Interrupt for direction sensor
ISR(INT1_vect){
 static unsigned long last_interrupt_time1 = 0;
 unsigned long interrupt_time1 = millis();
 if (interrupt_time1 - last_interrupt_time1 > 50){
  if(PIND & (1<<PD6)){                              // If sensor 2 direction HIGH
    if (count2 != 50)                               // Do not go above 50
     count2++; 
  }                                                 // Increment sensor 2 counter
  else if((PIND & (1<<PD6)) == 0){                  // If sensor 2 direction LOW
    if(count2 != 0)                                 // Do not go below 0
     count2--;                                      // Decrement sensor 2 counter
     }      
 last_interrupt_time1 = interrupt_time1;
 }
}


// DOES NOT WORK, for some reason cannot use the SS pin when using wireless module, even though its SS is on a different pin
//ISR(INT1_vect){
// static unsigned long last_interrupt_time1 = 0;
// unsigned long interrupt_time1 = millis();
// if (interrupt_time1 - last_interrupt_time1 > 100){
//  if(PINB & (1<<PB2)){                            // If sensor 2 direction HIGH
//     count2++; }                                  // Increment sensor 2 counter
//  else if((PINB & (1<<PB2)) == 0){                // If sensor 2 direction LOW
//    if(count2 != 0)
//     count2--;                                    // Decrement sensor 1 counter
//     }      
// last_interrupt_time1 = interrupt_time1;
// }
//}
