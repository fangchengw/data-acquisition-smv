

#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
#include <SPI.h>
#include <SD.h>

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 20, 4); // Change to (0x27,16,2) for 16x2 LCD.

#define BRIGHTNESS_PIN      9   // Must be a PWM pin
#define hall_pin 2
#define temp_pin A3
#define switch_pin 3
#define current_pin A0
#define soc_pin A1

//modify these parameters
int dataCollectionRate = 800; //data collection period (ms)and screen refresh rate
int brightness = 120; // brightness of lcd 0-255
float maxCurrent = 5;



int rpm = 0;

//variables for potentiometer
float voltage = 0;

//variables for timer
unsigned long timer = 0;
unsigned long startTime = 0;
int runnum;


float current = 0;
float soc = 0;


//file
File myFile;

void setup() {
  // Initiate the LCD:
  lcd.init();
  lcd.backlight();
  analogWrite(BRIGHTNESS_PIN, brightness);

  // initiate the potentiometer
  pinMode(temp_pin, INPUT);
  
  pinMode(switch_pin, INPUT);
  pinMode(current_pin, INPUT);
  pinMode(soc_pin, INPUT);

  //Serial.begin(9600);

  resetVar();
}
void loop() {
  //hall sensor
  /*
  pinValue = digitalRead(hall_pin);
  if (prevValue == initialPinValue && pinValue == (1 - initialPinValue)) {
    if (endTime == 0) {
      endTime = millis();
    } else {
      startTime = endTime;
      endTime = millis();
    }
    elapsedTime = endTime - startTime;
    rpm = 60000/elapsedTime;
    avgRpm = (avgRpm + rpm) / 2;
  }
  prevValue = pinValue;
  
  Serial.print("RPM: ");
  Serial.print(rpm);
  Serial.print("     Average RPM: ");
  Serial.print(avgRpm);
  Serial.println();*/
  /*
  int rawV = analogRead(temp_pin);
  float mV = (rawV / 1024.0) * 5000;
  float cel = mV / 10;
  float fahr = ( 9 / 5.0 ) * cel + 32;
  Serial.print(cel);
  Serial.print("  ");
  Serial.println(fahr);*/
  
  //soc = analogRead(soc_pin) / 1024.0 * 100.0;
  //current = maxCurrent * analogRead(current_pin) / 1024.0;



  //get data
  timer = millis() - startTime;
  
  if (digitalRead(switch_pin)) {
    //close file
    myFile.close();
    lcd.clear();
    lcd.setCursor(0, 1);
    lcd.print(millis());
    lcd.setCursor(0, 0);
    lcd.print("File Closed");
    delay(2000);
    resetVar();
  }
  
  soc = 1;
  current = 2;
  float fahr = 3;
  rpm =4;
  if (timer % 10000 == 0) {
    myFile.flush();
  }
  if (timer % dataCollectionRate == 0) {
      //values on lcd
      lcd.setCursor(0, 0); 
      String firstline = "R" + String(runnum) + " Time:" + String(timer / 1000.0);
      lcd.print(firstline); // Print the string "Hello World!"
      //Serial.print(timer);
      //Serial.print("  ");
      //Serial.println(timer/1000.0 );
          
      lcd.setCursor(0, 1); 
      String secondline = "Temp:" + String(fahr) + " SOC:" + String(soc);
      lcd.print(secondline);
    
      lcd.setCursor(0, 2);
      String thirdline = "RPM:" + String(rpm);
      lcd.print(thirdline);
    
      lcd.setCursor(0, 3);
      String forthline = "Current:" + String(current);
      lcd.print(forthline);

      //print value to file
      String toFile = String(timer / 1000.0) + "," + String(fahr) + "," + String(rpm) + "," + String(current) + "," + String(soc) + "," + String(millis() / 1000.0) + "\n";
      myFile.print(toFile);
  }

}

void doNothing() {
  while (true) {
    void();
  }
}

int getHeaderNum() {
  if (SD.exists("header.txt")) {
    int num = 1;
    File readfile = SD.open("header.txt", FILE_READ);
    while (readfile.available()) {
      num = readfile.read() - '0';
      //Serial.print(num);
    }
    
    return num + 1;
  } else {
    return 1;
  }
}
int writeToHeader(int headerNo) {
  File header = SD.open("header.txt", FILE_WRITE);
  if(header) {
    header.print(headerNo);
    header.close();
    return 0;
  } else {
    header.close();
    return -1;
  }
}
//reset all variable and wait for the button press to begin opening file
void resetVar() {
  lcd.clear();
  voltage = 0;
  soc = 0;
  current = 0;
  timer = 0;
  lcd.setCursor(0, 0);
  lcd.print("Hold To Begin");
  bool breakLoop = false;
  while (!breakLoop) {
    if (digitalRead(switch_pin)) {
      int index = 0;
      while (true) {
        //initiate SD card and the file
        SD.begin(4);
        runnum = getHeaderNum();
        //Serial.print(runnum);
        myFile = SD.open("Run_" + String(runnum) +".csv", FILE_WRITE); 
        if (myFile) {
          //Serial.print("Success in Opening Files");
          //TODO: add hear file for run number. display it somewhere and use it for file name
          if(writeToHeader(runnum) == -1) {
            goto failure;
          }
          String header = "Time,Temp,RPM,Current,State of Charge,Timer\n";
          myFile.print(header);
          break;
        } else {
          // if the file didn't open, print an error:
          goto failure;
        }
        failure:
        lcd.setCursor(0, 0);
        lcd.print("Error Opening File");
        lcd.setCursor(0, 1);
        lcd.print("Trying To Reopen");
        lcd.setCursor(0, 3);
        lcd.print("Hold To Restart");
        lcd.setCursor(index, 2);
        lcd.print(".");
        index++;
        if (index >= 20) {
          index = 0;
          lcd.setCursor(0, 2);
          lcd.print("                    ");
        }
        //Serial.println("error opening test.txt");
        //delay(500);
        if (digitalRead(switch_pin)) {
          lcd.clear();
          lcd.print("Restarting");
          setup();
          return; 
        }
       
        delay(500);
      }
      breakLoop = true;
    }
  }
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Start in 3...");
  delay(1000);
  lcd.print("2...");
  delay(1000);
  lcd.print("1..");
  delay(1000);
  startTime = millis();
  lcd.clear();
}
int saveinbackground() {
  
}
