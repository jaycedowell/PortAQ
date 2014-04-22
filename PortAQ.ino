#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

#include <stdlib.h>
#include <math.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #define statements make it easy to set the backlight color
#define ON 0x1
#define OFF 0x0

// Backlight state (1 = on)
int blStatus = 1;

// Status indicator LEDs, active = measuring, on = unit on, backlight off
int ledActive=6;
int ledOn=5;

// Setup for the dust sensor
int dustPin=0;
int ledPower=2;
int delayTime=280;
int delayTime2=40;
int offTime=9680;

// Setup for the gas sensors
int no2Pin=2;
int o3Pin=1;

// Menu location (0 = dust, 1 = NO2, or 2 = O3)
int displayMode = 0;

// Timer value in ms for loop/measurement control
long tStart;

// Pointers to store the sensor data
float *d, *n, *o;


/*
 Setup
*/

void setup() {
  // Setup the serial interface for reporting data to a computer
  Serial.begin(9600);
  
  // Setup the LEDs that we will be using
  pinMode(ledPower, OUTPUT);
  pinMode(ledActive, OUTPUT);
  pinMode(ledOn, OUTPUT);
    
  // Set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  lcd.setBacklight(ON);
  
  // Initialize the time comparizon
  tStart = millis();
  
  // Initalize the data arrays
  d = (float *) malloc(3*sizeof(float));
  n = (float *) malloc(3*sizeof(float));
  o = (float *) malloc(3*sizeof(float));
}


/*
 Main loop
*/

void loop() {
  char s[32];
  int update = 0;
  String line1;
  String line2;
  String response;
  
  // Update the sensor values every two seconds
  if( millis() - tStart > 2000 ) {
    digitalWrite(ledActive, HIGH);
    
    readDust(d);
    readNO2(n);
    readO3(o);
    
    tStart = millis();
    update = 1;
    
    // Report the data over serial
    //// Dust sensor reading
    response = "";
    response += "Dust, ";
    response += "Voltage: ";
    response += dtostrf(*(d + 0), 6, 4, s);
    response += ", Density: ";
    response += dtostrf(*(d + 1), 6, 4, s);
    response += ", Particles per 0.01 cubic foot: ";
    response += dtostrf(*(d + 2), 8, 0, s);
    Serial.println(response);
    
    //// NO2 reading
    response = "";
    response += "NO2, ";
    response += "Voltage: ";
    response += dtostrf(*(n + 0), 6, 4, s);
    response += ", Resistance: ";
    response += dtostrf(*(n + 1), 8, 0, s);
    response += ", PPM: ";
    response += dtostrf(*(n + 2), 6, 2, s);
    Serial.println(response);
    
    //// O3 reading
    response = "";
    response += "O3, ";
    response += "Voltage: ";
    response += dtostrf(*(o + 0), 6, 4, s);
    response += ", Resistance: ";
    response += dtostrf(*(o + 1), 8, 0, s);
    response += ", PPM: ";
    Serial.println(response);
    
    digitalWrite(ledActive, LOW);
  }
  
  // Check the button status
  uint8_t buttons = lcd.readButtons();
  
  if( buttons ) {
    // Backlight on/off
    if( buttons & BUTTON_SELECT ) {
      if( blStatus ) {
        lcd.setBacklight(OFF);
        digitalWrite(ledOn, HIGH);
        blStatus = 0;
      } else {
        lcd.setBacklight(ON);
        digitalWrite(ledOn, LOW);
        blStatus = 1;
      } 
    }
    // Advance mode - backward
    if( buttons & BUTTON_LEFT || buttons & BUTTON_DOWN ) {
      displayMode -= 1;
      if( displayMode < 0 ) {
        displayMode = 2;
      }
      update = 1;
    }
    // Advance mode - forward
    if( buttons & BUTTON_RIGHT || buttons & BUTTON_UP) {
      displayMode += 1;
      if( displayMode > 2 ) {
        displayMode = 0;
      }
      update = 1;
    }
  }
  
  // Update the LCD
  if( update ) {
    line1 = "";
    line2 = "";
    if( displayMode == 0 ) {
      // Dust
      line1 += "Dust: ";
      line1 += dtostrf(*(d + 1), 4, 2, s);
      line1 += " mg/m3";
      line2 += dtostrf(*(d + 2)*100/1000.0, 5, 1, s);
      line2 += " kp/cf     ";
    } else if( displayMode == 1 ) {
      // NO2
      line1 += "NO2: ";
      line1 += dtostrf(*(n + 1)/1000, 4, 1, s);
      line1 += " kOhm  "; 
      line2 += dtostrf(*(n + 2), 6, 4, s);
      line2 += " ppm      ";
    } else {
      // O3
      line1 += "O3: ";
      line1 += dtostrf(*(o + 1)/1000, 4, 1, s);
      line1 += " kOhm   "; 
      line2 += dtostrf(*(o + 2), 6, 4, s);
      line2 += " ppm      ";
    }
    lcd.setCursor(0,0);
    lcd.print(line1);
    lcd.setCursor(0,1);
    lcd.print(line2);
  }
  
  // Sleep for a bit
  if( buttons ) {
    delay(150);
  } else {
    delay(50);
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

void readDust(float *data) {
  int i;
  int dustVal;
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
  
  *(data + 0) = voltage;
  *(data + 1) = dustdensity;
  *(data + 2) = ppmpercf;
}


/*
 Interface to SGX Sensortech MiCS-2710 nitrogen dioxide sensor
 
 Pin 1: Arduino A2 pin and GND via a 10K ohm resistor
 Pin 2: 3.3V via a 47 ohm resistor
 Pin 3: 3.3V
 Pin 4: GND
*/

void readNO2(float *data) {
  int i;
  int gasVal;
  float voltage;
  float resistance;
  float ppm;
  
  gasVal = 0;
  for(i=0; i<10; i++) {
    gasVal += analogRead(no2Pin); // read the NO2 value
    delayMicroseconds(offTime);
  }
  
  // Average ADC counts -> voltage
  voltage = 5.0*gasVal/10.0/1024.0;
  // Convert the voltage to a resistance
  resistance = (3.3/voltage - 1)*10000.0;
  // Convert the resistance to a concentration - sensor specific
  // R @ fresh air is 6710 ohm
  ppm = 0.0353*pow(resistance/6710.0, 0.540);
  
  *(data + 0) = voltage;
  *(data + 1) = resistance;
  *(data + 2) = ppm;
}


/*
 Interface to SGX Sensortech MiCS-2710 ozone sensor
 
 Pin 1: Arduino A1 pin and GND via a 10K ohm resistor
 Pin 2: 3.3V via a 33 ohm resistor
 Pin 3: 3.3V
 Pin 4: GND
*/

void readO3(float *data) {
  int i;
  int gasVal;
  float voltage;
  float resistance;
  float ppm;
  
  gasVal = 0;
  for(i=0; i<10; i++) {
    gasVal += analogRead(o3Pin); // read the O3 value
    delayMicroseconds(offTime);
  }
  
  // Average ADC counts -> voltage
  voltage = 5.0*gasVal/10.0/1024.0;
  // Convert the voltage to a resistance
  resistance = (3.3/voltage - 1)*10000.0;
  // Convert the resistance to a  - sensor specific
  // R @ 100 ppb is 75970 ohm
  ppm = 10.05*pow(resistance/75970.0, 2) + 80.5*resistance/75970.0 + 4.54;
  ppm = ppm / 1000;
  
  *(data + 0) = voltage;
  *(data + 1) = resistance;
  *(data + 2) = ppm;
}
