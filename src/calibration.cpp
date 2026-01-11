/*
 Example using the SparkFun HX711 breakout board with a scale
 By: Nathan Seidle
 SparkFun Electronics
 Date: November 19th, 2014
 License: This code is public domain but you buy me a beer if you use this and we meet someday (Beerware license).
 
 This is the calibration sketch. Use it to determine the calibration_factor that the main example uses. It also
 outputs the zero_factor useful for projects that have a permanent mass on the scale in between power cycles.
 
 Setup your scale and start the sketch WITHOUT a weight on the scale
 Once readings are displayed place the weight on the scale
 Press +/- or a/z to adjust the calibration_factor until the output readings match the known weight
 Use this calibration_factor on the example sketch
 
 This example assumes pounds (lbs). If you prefer kilograms, change the Serial.print(" lbs"); line to kg. The
 calibration factor will be significantly different but it will be linearly related to lbs (1 lbs = 0.453592 kg).
 
 Your calibration factor may be very positive or very negative. It all depends on the setup of your scale system
 and the direction the sensors deflect from zero state

 This example code uses bogde's excellent library: https://github.com/bogde/HX711
 bogde's library is released under a GNU GENERAL PUBLIC LICENSE

 Arduino pin 2 -> HX711 CLK
 3 -> DOUT
 5V -> VCC
 GND -> GND
 
 Most any pin on the Arduino Uno will be compatible with DOUT/CLK.
 
 The HX711 board can be powered from 2.7V to 5V so the Arduino 5V power should be fine.
 
*/
#include <Arduino.h>
#include "HX711.h"

//GPIOs (note these don't match up with the silkscreen digital callouts on the XIAO board, see schematic)
const int IN1MotorPin = 44; //change to 44 if motor runs in reverse
const int IN2MotorPin = 7; //change to 7 if motor runs in reverse
const int buttonUPpin = 5; //labeled as D4 on the silkscreen for xiao
const int buttonDOWNpin = 6; //labeled as D5 on the slikscreen for xiao
const int HX_DOUT = 9; //hx711, labeled as D10 on the silkscreen for xiao
const int HX_CLK = 8; //hx711, labeled as D9 on the silkscreen for xiao

HX711 scale;

// float calibration_factor = -2000; //-19.9g, zero factor ~-340528
// float calibration_factor = -5000; //-8g, zero factor ~-340528
// float calibration_factor = -10000; //-4g, zero factor ~-340528
// float calibration_factor = -100000; //-0.4g, zero factor ~-340528
float calibration_factor = -1000; 

void setup() {
  Serial.begin(9600);
  Serial.println("HX711 calibration sketch");
  Serial.println("Remove all weight from scale");
  Serial.println("After readings begin, place known weight on scale");
  Serial.println("Press + or a to increase calibration factor");
  Serial.println("Press - or z to decrease calibration factor");
  
  // disable motor driver
  pinMode(IN1MotorPin, OUTPUT);
  pinMode(IN2MotorPin, OUTPUT);
  analogWrite(IN1MotorPin, 0);
  analogWrite(IN2MotorPin, 0);

  scale.begin(HX_DOUT,HX_CLK);
  scale.set_scale();
  scale.tare();	//Reset the scale to 0

  long zero_factor = scale.read_average(); //Get a baseline reading
  Serial.print("Zero factor: "); //This can be used to remove the need to tare the scale. Useful in permanent scale projects.
  Serial.println(zero_factor);
}

void loop() {

  scale.set_scale(calibration_factor); //Adjust to this calibration factor

  Serial.print("Reading: ");
  Serial.print(scale.get_units(), 1);
  Serial.print(" g"); //Change this to kg and re-adjust the calibration factor if you follow SI units like a sane person
  Serial.print(" calibration_factor: ");
  Serial.print(calibration_factor);
  Serial.println();

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+' || temp == 'a')
      calibration_factor += 10;
    else if(temp == '-' || temp == 'z')
      calibration_factor -= 10;
    else if(temp == '*')
      scale.tare();
  }
  delay(10);
}
