#include <stdlib.h>

int dustPin=0;
int ledPower=2;
int delayTime=280;
int delayTime2=40;
int offTime=9680;

void setup() {
  Serial.begin(9600);
  pinMode(ledPower, OUTPUT);
}

void loop() {
  char c;
  char s[32];
  String command;
  String response;
  
  command = "";
  response = "";
  
  // Read in a command
  while( Serial.available() > 0 )  {
    c = (char) Serial.read();
    command += c;
    delay(50);
  }
  
  // Run the command
  if( command.length() > 0 ) {
    if( command == "dust" ) {
      //// Dust sensor reading
      response = readDust();
    }
    if( command == "echo" ) {
      //// Echo
      response = command;
    }
    
    // Return the result
    Serial.println(response);
    delay(500);
  }
  
}

/*
 Interface to Sharp GP2Y1010AU0F Particle Sensor
 Program by Christopher Nafis 
 Written April 2012
 
 http://www.sparkfun.com/datasheets/Sensors/gp2y1010au_e.pdf
 http://sensorapp.net/?p=479
 
 Sharp pin 1 (V-LED)   => 5V (connected to 150ohm resister)
 Sharp pin 2 (LED-GND) => Arduino GND pin
 Sharp pin 3 (LED)     => Arduino pin 2
 Sharp pin 4 (S-GND)   => Arduino GND pin
 Sharp pin 5 (Vo)      => Arduino A0 pin
 Sharp pin 6 (Vcc)     => 5V
 */

String readDust() {
  int i;
  int dustVal;
  char s[32];
  float voltage;
  float dustdensity;
  float ppmpercf;
  
  dustVal = 0;
  for(i=0; i<10; i++) {
    digitalWrite(ledPower, LOW); // power on the LED
    delayMicroseconds(delayTime);
    dustVal += analogRead(dustPin); // read the dust value
    delayMicroseconds(delayTime2);
    digitalWrite(ledPower, HIGH); // turn the LED off
    delayMicroseconds(offTime);
  }

  voltage = 5.0*dustVal/10.0/1024.0;
  dustdensity = 0.17*voltage-0.1;
  ppmpercf = (voltage-0.0256)*120000;
  if( ppmpercf < 0 ) {
    ppmpercf = 0;
  }
  if( dustdensity < 0 ) {
    dustdensity = 0;
  }
  if(dustdensity > 0.5 ) {
    dustdensity = 0.5;
  }
  
  String dataString = "";
  dataString += "Voltage:";
  dataString += dtostrf(voltage, 6, 4, s);
  dataString += ", ";
  dataString += "Dust Density:";
  dataString += dtostrf(dustdensity, 6, 4, s);
  dataString += ", ";
  dataString += "Particles per 0.01 cubic foot:";
  dataString += dtostrf(ppmpercf, 8, 0, s);
  
  return dataString;
}

