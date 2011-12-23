//read PPM signals from 2 channels of an RC reciever and convert the values to PWM in either direction.
// digital pins 5 & 9 control motor1, digital pins 6 & 10 control motor2. DP 12 and 13 are neutral indicator lights. DP 2 and 3 are inputs from the R/C receiver. All analog pins are open. When motor pin is HIGH, bridge is open. All mosfets are pulled up/down to stay closed unless told to open.


// Timer1 is set to 32kHz (pwm pins 9 and 10)

//new code testing: bring pin 7 high to run alternate functions. No speed control mode, fully on or fully off.

#include "WProgram.h"
void setup();
void rc1();
void rc2();
void loop();
int fx_pin = 7;

int motor1_a = 5;
int motor1_b = 6;

int motor2_a = 9;
int motor2_b = 10;

//Neutral indicator LED's (FWD and REV LED's are on the motor driver board.
int ledPin1 = 12;
int ledPin2 = 13;

int ppm1 = 2;  // connect the desired channel (PPM signal) from your RC receiver to digital pin 2 on Arduino.
int ppm2 = 3;

unsigned long rc1_PulseStartTicks;
volatile int rc1_val;  // store RC signal pulse length
int lastgood1;
int adj_val1;  // mapped value to be between 0-511
int rc1_InputReady;

unsigned long rc2_PulseStartTicks;
volatile int rc2_val;  // store RC signal pulse length
int lastgood2;
int adj_val2;  // mapped value to be between 0-511
int rc2_InputReady;

int deadband_high = 265; // set the desired deadband ceiling (ie. if adj_val is above this, you go forward)
int deadband_low = 245;  // set deadband floor (ie. below this, go backward)

int pwm_ceiling = 256; // adjusts speed of the PWM signal when it goes above the forward threshold (deadband_high). f(x) = adj_value1 (let's say it is max 511) - pwm_ceiling (256) = PWM write value (255)
int pwm_floor = 255;  // adjusts speed of the PWM signal when it goes below the reverese threshold (deadband_low). f(x) = pwm_floor (255) - adj_value1 (let's say it is min 0) = PWM write value (255)
 
int delta_val = 400; //this value sets the range for discarding unwanted signals.

int max_rc = 511;
int min_rc = 0;
int max_rc2 = max_rc;
int min_rc2 = min_rc;

void setup() {

  TCCR1B = TCCR1B & 0b11111000 | 0x01;
  //TCCR2B = TCCR2B & 0b11111000 | 0x01;
  
  Serial.begin(9600);

  // set all the other pins you're using as outputs:

  pinMode(fx_pin, INPUT);

  //motor1 pins
  pinMode(motor1_a, OUTPUT);
  pinMode(motor1_b, OUTPUT);

  pinMode(motor2_a, OUTPUT);
  pinMode(motor2_b, OUTPUT);

  //led's
  pinMode(ledPin1, OUTPUT);
  pinMode(ledPin2, OUTPUT);

  //PPM inputs from RC receiver
  pinMode(ppm1, INPUT); //Pin 2 as input
  pinMode(ppm2, INPUT); //Pin 3 as input

  //attachInterrupt(0, rc1, CHANGE);    // catch interrupt 0 (digital pin 2) going HIGH and send to rc1()
  attachInterrupt(1, rc2, CHANGE);    // catch interrupt 1 (digital pin 3) going HIGH and send to rc2()

 
  lastgood1 = 255; // default to neutral
  lastgood2 = 255;
  
}


void rc1()
{
  // did the pin change to high or low?
  if (digitalRead( ppm1 ) == HIGH)
  {
    // store the current micros() value
    rc1_PulseStartTicks = micros();
  }
  else
  {
    // Pin transitioned low, calculate the duration of the pulse
    rc1_val = micros() - rc1_PulseStartTicks; // may glitch during timer wrap-around
    // Set flag for main loop to process the pulse
    rc1_InputReady = true;
  }
}

void rc2()
{
  // did the pin change to high or low?
  if (digitalRead( ppm2 ) == HIGH)
  {
    // store the current micros() value
    rc2_PulseStartTicks = micros();
  }
  else
  {
    // Pin transitioned low, calculate the duration of the pulse
    rc2_val = micros() - rc2_PulseStartTicks; // may glitch during timer wrap-around
    // Set flag for main loop to process the pulse
    rc2_InputReady = true;
  }
}


void loop() {



//////////////////////////////////signal1
    if (rc1_InputReady)
  {
    // reset input flag
    rc1_InputReady = false;

    // constrain and map the pulse length
    adj_val1 = map(constrain(rc1_val, 1000, 2000), 1000, 2000, 0, 511);

    if (adj_val1 == 0){	    // if value is 0, use last good value
	adj_val1 = lastgood1;
    }
    else if (adj_val1 == 511){   // if value is 511, use last good value
	adj_val1 = lastgood1;
    }
    else if (((adj_val1 - lastgood1) * 2) > delta_val){   // make sure the new value is within a reasonable range of the last known good value.
	adj_val1 = lastgood1;
    }
    else {
	lastgood1 = adj_val1;   // if all conditions are met, use new value and set it as lastgood1 value.
    }

    // Update motor outputs.


///////// MOTOR1 /////////
if (digitalRead(fx_pin) == HIGH) {
  if (adj_val1 > deadband_high) {
    //Forward
    digitalWrite(motor1_b, LOW);
    digitalWrite(motor1_a, HIGH); 
    digitalWrite(ledPin1, LOW);
  }
  else if (adj_val1 < deadband_low) {
    //REVERSe
    digitalWrite(motor1_a, LOW);
    digitalWrite(motor1_b, HIGH); 
    digitalWrite(ledPin1, LOW);
  }
  else {
  //STOP
  digitalWrite(motor1_a, LOW);
  digitalWrite(motor1_b, LOW);
  digitalWrite(ledPin1, HIGH); // turn on neutral light, turn motor pins off
 }
}

else {

  if (adj_val1 > deadband_high) {
    //Forward
    digitalWrite(motor1_b, LOW);
    analogWrite(motor1_a, adj_val1 - pwm_ceiling); //write adj_val1 to motor1
//    analogWrite(motor1_a, 511 - adj_val1); // .... inverted formula
    digitalWrite(ledPin1, LOW);
  }
  else if (adj_val1 < deadband_low) {
    //REVERSE
    digitalWrite(motor1_a, LOW);
    analogWrite(motor1_b, pwm_floor - adj_val1); //write adj_val1 to motor1
//    analogWrite(motor1_b, adj_val1); // .... inverted formula
    digitalWrite(ledPin1, LOW);
  }
  else {
  //STOP
  digitalWrite(motor1_a, LOW);
  digitalWrite(motor1_b, LOW);
  digitalWrite(ledPin1, HIGH); // turn on neutral light, turn motor pins off
 }
}

}

//////////////////////////////////signal2
    if (rc2_InputReady)
  {
    // reset input flag
    rc2_InputReady = false;

    // constrain and map the pulse length
    adj_val2 = map(constrain(rc2_val, 1000, 2000), 1000, 2000, min_rc2, max_rc2);

    if (adj_val2 == 0){	    // if value is 0, use last good value
	adj_val2 = lastgood2;
    }
    else if (adj_val2 == 511){   // if value is 511, use last good value
	adj_val2 = lastgood2;
    }
    else if (((adj_val2 - lastgood2) * 2) > delta_val){   // make sure the new value is within a reasonable range of the last known good value.
	adj_val2 = lastgood2;
    }
    else {
	lastgood2 = adj_val2;   // if all conditions are met, use new value and set it as lastgood1 value.
    }

    // Update motor outputs.



///////// MOTOR2 /////////

if (digitalRead(fx_pin) == HIGH) {

  digitalWrite(motor1_a, LOW);
  digitalWrite(motor1_b, LOW);
  digitalWrite(motor2_a, LOW);
  digitalWrite(motor2_b, LOW);
  
  if (max_rc == 511){
    max_rc = 300;
  }
  if (min_rc == 0){
    min_rc = 200;
  }
     if (adj_val2 > max_rc) {
       max_rc = adj_val2;
     }
     else if (adj_val2 < min_rc) {
       min_rc = adj_val2;
     }
     else {
     }



}


else {

  if (adj_val2 > deadband_high) {
    //Forward
    digitalWrite(motor2_b, LOW);
    analogWrite(motor2_a, adj_val2 - pwm_ceiling); //write adj_val1 to motor1
//    analogWrite(motor2_a, 511 - adj_val2); // .... inverted formula
    digitalWrite(ledPin2, LOW);
  }
  else if (adj_val2 < deadband_low) {
    //REVERSE
    digitalWrite(motor2_a, LOW);
    analogWrite(motor2_b, pwm_floor - adj_val2); //write adj_val1 to motor1
//    analogWrite(motor2_b, adj_val2); // .... inverted formula
    digitalWrite(ledPin2, LOW);
  }
  else {
  //STOP
  digitalWrite(motor2_a, LOW);
  digitalWrite(motor2_b, LOW);
  digitalWrite(ledPin2, HIGH); // turn on neutral light, turn motor pins off
  }
  
 }

}


  //print values
  Serial.print("channel 1:  ");
  Serial.print(adj_val1);
  Serial.print("	  ");
  Serial.print("rc1_val raw:  ");
  Serial.print(rc1_val);
  Serial.print("	  ");
  Serial.print("channel 2:  ");
  Serial.print(adj_val2);
  Serial.print("	  ");
  Serial.print("rc2_val raw:  ");
  Serial.print(rc2_val);
  Serial.print("	  ");
  Serial.print("max_rc:  ");
  Serial.print(max_rc);
  Serial.print("	  ");
  Serial.print("min_rc:  ");
  Serial.print(min_rc);  
  Serial.println("	  ");


}



int main(void)
{
	init();

	setup();
    
	for (;;)
		loop();
        
	return 0;
}

