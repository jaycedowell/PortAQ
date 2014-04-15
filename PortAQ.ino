#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

#include <stdlib.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define ON 0x1
#define OFF 0x0

int dustPin=0;
int ledPower=2;
int delayTime=280;
int delayTime2=40;
int offTime=9680;

int no2Pin=1;
int o3Pin=1;

int blStatus = 0;
int displayMode = 1;

void setup() {
  Serial.begin(9600);
  pinMode(ledPower, OUTPUT);
  
  // Set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
}

void loop() {
  char c;
  char s[32];
  float *d, *n, *o;
  String command;
  String response;
  
  // Update the sensor values
  d = (float *) malloc(3*sizeof(float));
  readDust(d);
  n = (float *) malloc(3*sizeof(float));
  readNO2(n);
  o = (float *) malloc(3*sizeof(float));
  readO3(o);
  
  // Check the button status
  uint8_t buttons = lcd.readButtons();
  
  if( buttons ) {
    lcd.setCursor(0,0);
    // Backlight on/off
    if( buttons & BUTTON_SELECT ) {
      if( blStatus ) {
        lcd.setBacklight(OFF);
        blStatus = 0;
      } else {
        lcd.setBacklight(ON);
        blStatus = 1;
      } 
    }
    // Advance mode - backward
    if( buttons & BUTTON_LEFT ) {
      displayMode -= 1;
      if( displayMode < 0 ) {
        displayMode = 2;
      }
    }
    // Advance mode - forward
    if( buttons & BUTTON_RIGHT ) {
      displayMode += 1;
      if( displayMode > 2 ) {
        displayMode = 0;
      }
    }
  }
  
  // Update the LCD
  lcd.setCursor(0,0);
  response = "";
  if( displayMode == 0 ) {
    // Dust
    response += "Dust:\n";
    response += dtostrf(*(d + 2), 8, 0, s);
    response += "part/0.01 cf";
  } else if( displayMode == 1 ) {
    // NO2
    response += "NO2:\n";
    response += dtostrf(*(n + 0), 8, 0, s);
    response += "V";
  } else {
    // O3
    response += "O3:\n";
    response += dtostrf(*(o + 0), 8, 0, s);
    response += "V";
  }
  lcd.print(response);
  
  // Read in a command
  command = "";
  while( Serial.available() > 0 )  {
    c = (char) Serial.read();
    command += c;
    delay(50);
  }
  
  // Run the command
  if( command.length() > 0 ) {
    response = "";
    if( command == "dust" ) {
      //// Dust sensor reading
      response += "Voltage: ";
      response += dtostrf(*(d + 0), 6, 4, s);
      response += "Dust Density: ";
      response += dtostrf(*(d + 1), 6, 4, s);
      response += "Particles per 0.01 cubic foot: ";
      response += dtostrf(*(d + 2), 8, 0, s);
    } else if( command == "no2" ) {
      //// NO2 reading
      response += "NO2: ";
      response += "Voltage: ";
      response += dtostrf(*(n + 0), 6, 4, s);
      response += "Resistance: ";
      response += dtostrf(*(n + 1), 8, 0, s);
      response += "PPM: ";
      response += dtostrf(*(n + 2), 6, 2, s);
    } else if( command == "o3" ) {
      //// O3 reading
      response += "O3: ";
      response += "Voltage: ";
      response += dtostrf(*(o + 0), 6, 4, s);
      response += "Resistance: ";
      response += dtostrf(*(o + 1), 8, 0, s);
      response += "PPM: ";
      response += dtostrf(*(o + 2), 6, 2, s);
    }
    
    // Return the result
    Serial.println(response);
  }
  
  free(d);
  free(n);
  free(o);
  
  delay(500);
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

void readNO2(float *data) {
  int i;
  int gasVal;
  float voltage;
  float resistance;
  float ppm;
  
  gasVal = 0;
  for(i=0; i<10; i++) {
    gasVal += analogRead(no2Pin); // read the NO2 value
  }
  
  voltage = 5.0*gasVal/10.0/1024.0;
  resistance = (3.3/voltage - 1)*10000.0;
  ppm = 1.0*voltage;
  
  *(data + 0) = voltage;
  *(data + 1) = resistance;
  *(data + 2) = ppm;
}

void readO3(float *data) {
  int i;
  int gasVal;
  float voltage;
  float resistance;
  float ppm;
  
  gasVal = 0;
  for(i=0; i<10; i++) {
    gasVal += analogRead(o3Pin); // read the O3 value
  }
  
  voltage = 5.0*gasVal/10.0/1024.0;
  resistance = (3.3/voltage - 1)*10000.0;
  ppm = 1.0*voltage;
  
  *(data + 0) = voltage;
  *(data + 1) = resistance;
  *(data + 2) = ppm;
}
